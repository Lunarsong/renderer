#include "detail/MaterialParams.h"

void MaterialParams::Destroy(MaterialParams* instance) {
  delete upcast(instance);
}

void MaterialParamsImpl::SetTexture(uint32_t binding,
                                    RenderAPI::ImageView texture) {
  params_[binding].texture = texture;
  params_[binding].dirty = true;
  dirty_ = true;
}

void MaterialParamsImpl::SetParam(uint32_t binding, const void* data) {
  memcpy(params_[binding].data.data.get(), data, params_[binding].data.size);
  params_[binding].dirty = true;
  dirty_ = true;
}

void MaterialParamsImpl::Commit() {
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

  // Update the descriptor set.
  RenderAPI::DescriptorSet old_set = set_;
  ++set_;
  RenderAPI::DescriptorSet new_set = set_;

  uint32_t binding = 0;
  for (auto& param : params_) {
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
          param.texture == RenderAPI::kInvalidHandle) {
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
  dirty_ = false;
}

const RenderAPI::DescriptorSet* MaterialParamsImpl::DescriptorSet() const {
  return set_;
}

MaterialParamsImpl::~MaterialParamsImpl() {
  for (auto& param : params_) {
    if (param.buffers != RenderAPI::kInvalidHandle) {
      param.buffers.Destroy();
    }
  }
}

// Map to implementation
void MaterialParams::SetTexture(uint32_t binding,
                                RenderAPI::ImageView texture) {
  upcast(this)->SetTexture(binding, texture);
}

void MaterialParams::SetParam(uint32_t binding, const void* data) {
  upcast(this)->SetParam(binding, data);
}

void MaterialParams::Commit() { upcast(this)->Commit(); }

const RenderAPI::DescriptorSet* MaterialParams::DescriptorSet() const {
  return upcast(this)->DescriptorSet();
}
