#include "detail/MaterialInstance.h"

void MaterialInstance::Destroy(MaterialInstance* instance) {
  delete upcast(instance);
}

void MaterialInstanceImpl::SetTexture(uint32_t set, uint32_t binding,
                                      RenderAPI::ImageView texture) {
  descriptors_[set].params[binding].texture = texture;
  descriptors_[set].params[binding].dirty = true;
  descriptors_[set].dirty = true;
  dirty_ = true;
}

void MaterialInstanceImpl::SetParam(uint32_t set, uint32_t binding,
                                    const void* data) {
  memcpy(descriptors_[set].params[binding].data.data.get(), data,
         descriptors_[set].params[binding].data.size);
  descriptors_[set].params[binding].dirty = true;
  descriptors_[set].dirty = true;
  dirty_ = true;
}

void MaterialInstanceImpl::Commit() {
  if (!dirty_) {
    return;
  }

  // Data to update.
  std::vector<RenderAPI::WriteDescriptorSet> writes;
  std::vector<RenderAPI::CopyDescriptorSet> copies;
  std::vector<RenderAPI::DescriptorImageInfo> images;
  images.reserve(10);
  std::vector<RenderAPI::DescriptorBufferInfo> buffers;
  buffers.reserve(10);
  for (auto& descriptor : descriptors_) {
    if (!descriptor.dirty) {
      continue;
    }
    // Update the descriptor set.
    RenderAPI::DescriptorSet old_set = descriptor.set;
    ++descriptor.set;
    RenderAPI::DescriptorSet new_set = descriptor.set;

    uint32_t binding = 0;
    for (auto& param : descriptor.params) {
      if (param.dirty) {
        param.dirty = false;
        RenderAPI::WriteDescriptorSet write;
        write.binding = binding;
        write.set = new_set;
        write.descriptor_count = 1;
        write.dst_array_element = 0;
        write.type = param.type;

        if (param.type == RenderAPI::DescriptorType::kUniformBuffer) {
          ++param.buffers;
          memcpy(RenderAPI::MapBuffer(param.buffers), param.data.data.get(),
                 param.data.size);
          RenderAPI::UnmapBuffer(param.buffers);
          write.buffers =
              &buffers.emplace_back(param.buffers, 0, param.data.size);
        } else if (param.type ==
                   RenderAPI::DescriptorType::kCombinedImageSampler) {
          write.images = &images.emplace_back(param.texture, param.sampler);
        }
        writes.emplace_back(std::move(write));
      } else {
        RenderAPI::CopyDescriptorSet copy;
        copy.src_set = old_set;
        copy.src_binding = binding;
        copy.src_array_element = 0;
        copy.dst_set = new_set;
        copy.dst_binding = binding;
        copy.dst_array_element = 0;
        if (param.type == RenderAPI::DescriptorType::kCombinedImageSampler &&
            param.texture == 0) {
          copy.descriptor_count = 0;
        } else {
          copy.descriptor_count = 1;
        }
        copies.emplace_back(std::move(copy));
      }
      ++binding;
    }

    RenderAPI::UpdateDescriptorSets(device_, writes.size(), writes.data(),
                                    copies.size(), copies.data());
    descriptor.dirty = false;
    writes.clear();
    copies.clear();
    images.clear();
    buffers.clear();
  }
  dirty_ = false;
}

const RenderAPI::DescriptorSet* MaterialInstanceImpl::DescriptorSet(
    uint32_t set) const {
  return descriptors_[set].set;
}

RenderAPI::ImageView MaterialInstanceImpl::GetTexture(uint32_t set,
                                                      uint32_t binding) {
  return descriptors_[set].params[binding].texture;
}

MaterialInstanceImpl::~MaterialInstanceImpl() {
  for (auto& set : descriptors_) {
    for (auto& param : set.params) {
      if (param.buffers != RenderAPI::kInvalidHandle) {
        param.buffers.Destroy();
      }
    }
  }
}

// Map to implementation
void MaterialInstance::SetTexture(uint32_t set, uint32_t binding,
                                  RenderAPI::ImageView texture) {
  upcast(this)->SetTexture(set, binding, texture);
}

void MaterialInstance::SetParam(uint32_t set, uint32_t binding,
                                const void* data) {
  upcast(this)->SetParam(set, binding, data);
}

void MaterialInstance::Commit() { upcast(this)->Commit(); }

const RenderAPI::DescriptorSet* MaterialInstance::DescriptorSet(
    uint32_t set) const {
  return upcast(this)->DescriptorSet(set);
}
RenderAPI::ImageView MaterialInstance::GetTexture(uint32_t set,
                                                  uint32_t binding) {
  return upcast(this)->GetTexture(set, binding);
}