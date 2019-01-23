#include "render_graph_builder.h"

#include <cassert>

RenderGraphBuilder::RenderGraphBuilder(RenderGraphCache* cache)
    : cache_(cache) {}

const RenderGraphResourceTextures& RenderGraphBuilder::UseRenderTarget(
    RenderGraphResource resource) {
  if (debug_current_pass_render_targets_++ > 0) {
    assert(false && "Must only have one render target per pass!");
  }

  render_targets_[resource].emplace_back(current_pass_);
  auto& it = framebuffers_.find(resource);
  assert(it != framebuffers_.cend() && "Invalid render target!");
  ++it->second.ref_count;
  for (auto texture : it->second.textures.textures) {
    Write(texture);
  }
  return it->second.textures;
}

const RenderGraphResourceTextures& RenderGraphBuilder::CreateRenderTarget(
    RenderGraphTextureDesc info) {
  RenderGraphFramebufferDesc framebuffer_info = {{info}};
  return CreateRenderTarget(framebuffer_info);
}

const RenderGraphResourceTextures& RenderGraphBuilder::CreateRenderTarget(
    RenderGraphFramebufferDesc info) {
  auto handle = handles_.Create();

  RenderGraphFramebufferResource resource;
  for (const auto& it : info.textures) {
    resource.textures.textures.emplace_back(CreateTexture(it));
  }
  resource.desc = std::move(info);
  framebuffers_[handle] = std::move(resource);

  return UseRenderTarget(handle);
}

RenderGraphResource RenderGraphBuilder::CreateTexture(
    RenderGraphTextureDesc info) {
  auto handle = handles_.Create();

  RenderGraphTextureResource resource;
  resource.desc = std::move(info);
  textures_[handle] = std::move(resource);
  return handle;
}

void RenderGraphBuilder::Write(RenderGraphResource resource) {
  writes_[resource].emplace_back(current_pass_);
}
void RenderGraphBuilder::Read(RenderGraphResource resource) {
  reads_[resource].emplace_back(current_pass_);
}

std::vector<RenderGraphNode> RenderGraphBuilder::Build(
    std::vector<RenderGraphPass>& passes) {
  // Handle aliasing.
  for (const auto& alias : aliases_) {
    const auto& writes = writes_.find(alias.first);
    if (writes != writes_.cend()) {
      auto& new_resource = writes_[GetAlised(alias.first)];
      new_resource.insert(new_resource.begin(), writes->second.begin(),
                          writes->second.end());
      writes_.erase(writes);
    }

    const auto& reads = reads_.find(alias.first);
    if (reads != reads_.cend()) {
      auto& new_resource = reads_[GetAlised(alias.first)];
      new_resource.insert(new_resource.begin(), reads->second.begin(),
                          reads->second.end());
      reads_.erase(reads);
    }

    const auto& render_targets = render_targets_.find(alias.first);
    if (render_targets != render_targets_.cend()) {
      auto& new_resource = render_targets_[GetAlised(alias.first)];
      new_resource.insert(new_resource.begin(), render_targets->second.begin(),
                          render_targets->second.end());
      render_targets_.erase(render_targets);
    }
  }

  // Create the resources.
  for (auto& it : textures_) {
    auto& texture = it.second;
    if (texture.transient) {
      texture.texture = cache_->CreateTransientTexture(texture.desc);
    }
  }

  for (auto& it : framebuffers_) {
    auto& framebuffer = it.second;
    for (auto& texture_handle : framebuffer.textures.textures) {
      texture_handle = GetAlised(texture_handle);
    }

    if (framebuffer.transient) {
      std::vector<Renderer::ImageView> images;
      images.reserve(framebuffer.textures.textures.size());
      for (const auto& texture_handle : framebuffer.textures.textures) {
        images.emplace_back(textures_[texture_handle].texture);
      }
      framebuffer.framebuffer =
          cache_->CreateTransientFramebuffer(framebuffer.desc, images);
    }
  }

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
      ++passes[writer].ref_count;
    }
  }

  // Cull writers without readers.
  for (const auto& writers : writes_) {
    const auto& readers = reads_.find(writers.first);
    if (readers == reads_.end()) {
      // No one is data, reduce refence count of writers.
      for (auto& writer : writers.second) {
        if (--passes[writer].ref_count == 0) {
          culled[writer] = true;
        }
      }
    }
  }

  for (auto& render_target : render_targets_) {
    RenderGraphNode node;
    RenderGraphCombinedRenderPasses render_node;
    render_node.framebuffer = framebuffers_[render_target.first].framebuffer;
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
    nodes[i - 1].signal_semaphores.emplace_back(semaphore);
    nodes[i].wait_semaphores.emplace_back(semaphore);
  }

  // Create the command buffers.
  for (auto& it : nodes) {
    it.render_cmd = cache_->AllocateCommand();
  }

  // Build scopes.
  for (const auto& readers : reads_) {
    for (auto reader : readers.second) {
      if (culled[reader]) {
        continue;
      }
      passes[reader].scope.textures_[readers.first] =
          textures_[readers.first].texture;
    }
  }

  return nodes;
}

void RenderGraphBuilder::Reset() {
  handles_.Reset();
  reads_.clear();
  writes_.clear();
  render_targets_.clear();
  framebuffers_.clear();
  textures_.clear();
}

void RenderGraphBuilder::SetCurrentPass(RenderGraphPassHandle pass) {
  current_pass_ = pass;
  debug_current_pass_render_targets_ = 0;
}

RenderGraphResource RenderGraphBuilder::GetAlised(
    RenderGraphResource handle) const {
  std::unordered_map<RenderGraphResource, RenderGraphResource>::const_iterator
      it = aliases_.find(handle);
  while (it != aliases_.cend()) {
    handle = it->second;
    it = aliases_.find(handle);
  }
  return handle;
}