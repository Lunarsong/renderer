#include "render_graph_builder.h"

#include <cassert>

RenderGraphBuilder::RenderGraphBuilder(RenderGraphCache* cache)
    : cache_(cache) {}

void RenderGraphBuilder::UseRenderTarget(RenderGraphResource resource) {
  render_targets_[resource].emplace_back(current_pass_);
  Write(resource);
}

void RenderGraphBuilder::Write(RenderGraphResource resource) {
  writes_[resource].emplace_back(current_pass_);
}
void RenderGraphBuilder::Read(RenderGraphResource resource) {
  reads_[resource].emplace_back(current_pass_);
}

void RenderGraphBuilder::Build(std::vector<RenderGraphPass>& passes) {
  std::vector<bool> culled(passes.size(), false);
  std::vector<uint32_t> pass_refs(passes.size(), 0);

  for (const auto& writers : writes_) {
    for (auto writer : writers.second) {
      ++pass_refs[writer];
    }
  }

  // Cull passes.
  for (const auto& readers : reads_) {
    const auto& writers = writes_.find(readers.first);
    if (writers == writes_.cend()) {
      // No one is writing to these pass. Cull it.
      for (auto reader : readers.second) {
        culled[reader] = true;
      }
    }
  }

  for (const auto& writers : writes_) {
    const auto& readers = reads_.find(writers.first);
    if (readers == reads_.end()) {
      // No one is data, reduce refence count of writers.
      for (auto& writer : writers.second) {
        if (--pass_refs[writer] == 0) {
          culled[writer] = true;
        }
      }
    }
  }

  // Create semaphores.
  for (auto& readers : reads_) {
    auto& writers = writes_.find(readers.first);
    if (writers == writes_.end()) {
      continue;
    }
    for (auto reader : readers.second) {
      if (culled[reader]) {
        continue;
      }

      for (auto writer : writers->second) {
        if (writer == reader) {
          continue;
        }
        Renderer::Semaphore semaphore = cache_->AllocateSemaphore();
        passes[writer].signal_semaphores.emplace_back(semaphore);
        passes[reader].wait_semaphores.emplace_back(semaphore);
      }
    }
  }

  for (auto& render_target : render_targets_) {
    auto* framebuffer = cache_->GetFrameBuffer(render_target.first);
    assert(framebuffer);

    for (auto& it : render_target.second) {
      if (culled[it]) {
        continue;
      }
      passes[it].render_pass = framebuffer->pass;
      passes[it].framebuffer = framebuffer->framebuffer;
    }
  }

  uint64_t i = 0;
  for (auto& it : passes) {
    if (culled[i]) {
      continue;
    }
    it.cmd = cache_->AllocateCommand();
    ++i;
  }
}

void RenderGraphBuilder::Reset() {
  reads_.clear();
  writes_.clear();
  render_targets_.clear();
}

void RenderGraphBuilder::SetCurrentPass(RenderGraphPassHandle pass) {
  current_pass_ = pass;
}