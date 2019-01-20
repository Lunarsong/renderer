#include "render_graph_builder.h"

#include <cassert>

RenderGraphBuilder::RenderGraphBuilder(RenderGraphCache* cache)
    : cache_(cache) {}

void RenderGraphBuilder::UseRenderTarget(RenderGraphResource resource) {
  render_targets_[resource].emplace_back(current_pass_);
  Write(resource);
  if (debug_current_pass_render_targets_++ > 0) {
    assert(false && "Must only have one render target per pass!");
  }
}

void RenderGraphBuilder::Write(RenderGraphResource resource) {
  writes_[resource].emplace_back(current_pass_);
}
void RenderGraphBuilder::Read(RenderGraphResource resource) {
  reads_[resource].emplace_back(current_pass_);
}

std::vector<RenderGraphNode> RenderGraphBuilder::Build(
    std::vector<RenderGraphPass>& passes) {
  // TODO:
  // 1. Optimize the culling algorithm.
  // 2. Add proper semaphore generation according to dependencies.
  // 3. Add support for non render pass.
  // 4. Add support for read only passes(?).
  std::vector<RenderGraphNode> nodes;

  // Cull unused render passes. (This is very rough and needs optimizing 💩.)
  std::vector<bool> culled(
      passes.size(),
      false);  // For later: Use a linear allocator or add it to the struct, or
               // a better way to remove the nodes.

  // Remove every reader who no one is writing to their resources.
  for (const auto& readers : reads_) {
    const auto& writers = writes_.find(readers.first);
    if (writers == writes_.cend()) {
      // No one is writing to these pass. Cull it.
      for (auto reader : readers.second) {
        culled[reader] = true;
      }
    }
  }

  // For every writer, add a reference count for each resource they write to.
  for (const auto& writers : writes_) {
    for (auto writer : writers.second) {
      ++passes[writer].count_ref;
    }
  }

  // Cull writers without readers.
  for (const auto& writers : writes_) {
    const auto& readers = reads_.find(writers.first);
    if (readers == reads_.end()) {
      // No one is data, reduce refence count of writers.
      for (auto& writer : writers.second) {
        if (--passes[writer].count_ref == 0) {
          culled[writer] = true;
        }
      }
    }
  }

  for (auto& render_target : render_targets_) {
    auto* framebuffer = cache_->GetFrameBuffer(render_target.first);
    assert(framebuffer);

    RenderGraphNode node;
    RenderGraphCombinedRenderPasses render_node;
    render_node.framebuffer = *framebuffer;
    for (auto& it : render_target.second) {
      if (culled[it]) {
        continue;
      }
      render_node.passes.emplace_back(&passes[it]);
    }
    if (!render_node.passes.empty()) {
      node.render_passes.emplace_back(std::move(render_node));
      nodes.emplace_back(std::move(node));
    }
  }

  // Create semaphores.
  /*for (auto& readers : reads_) {
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
  }*/
  // Create dumb semaphores for now (later optimize
  // semaphores to be created for dependencies).
  for (size_t i = 1; i < nodes.size(); ++i) {
    Renderer::Semaphore semaphore = cache_->AllocateSemaphore();
    nodes[i - 1].wait_semaphores.emplace_back(semaphore);
    nodes[i].wait_semaphores.emplace_back(semaphore);
  }

  // Create the command buffers.
  for (auto& it : nodes) {
    it.render_cmd = cache_->AllocateCommand();
  }

  return nodes;
}

void RenderGraphBuilder::Reset() {
  reads_.clear();
  writes_.clear();
  render_targets_.clear();
}

void RenderGraphBuilder::SetCurrentPass(RenderGraphPassHandle pass) {
  current_pass_ = pass;
  debug_current_pass_render_targets_ = 0;
}