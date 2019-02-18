#include "detail/Material.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include "BuilderBase.h"
#include "detail/MaterialInstance.h"
#include "detail/MaterialParams.h"

namespace {
struct SamplerInfo {
  RenderAPI::SamplerCreateInfo info;
  uint32_t index;
};

struct TextureInfo {
  uint32_t set;
  uint32_t binding;
  RenderAPI::ShaderStageFlags stages;
  const char* sampler;
  TextureInfo(uint32_t set, uint32_t binding,
              RenderAPI::ShaderStageFlags stages, const char* sampler)
      : set(set), binding(binding), stages(stages), sampler(sampler) {}
};
}  // namespace

struct Material::BuilderDetails {
  RenderAPI::Device device;
  Shading shading = Shading::kLit;
  BlendMode blending = BlendMode::kOpaque;
  RenderAPI::GraphicsPipelineCreateInfo info;
  RenderAPI::PipelineLayoutCreateInfo layout_info;
  std::vector<DescriptorBindings> descriptors;
  std::vector<TextureInfo> texture_to_sampler;
  std::vector<VertexAttribute> attributes;
  std::unordered_map<std::string, SamplerInfo> samplers;
  uint32_t num_uniform_buffers = 0;

  std::vector<uint8_t> frag_specialization_data;
  std::vector<uint8_t> vert_specialization_data;
  std::vector<RenderAPI::SpecializationMapEntry> frag_specialization_entries;
  std::vector<RenderAPI::SpecializationMapEntry> vert_specialization_entries;
};

Material::Builder::Builder(RenderAPI::Device device) { impl_->device = device; }

Material::Builder& Material::Builder::VertexCode(const uint32_t* code,
                                                 size_t code_size) {
  impl_->info.vertex.code = code;
  impl_->info.vertex.code_size = code_size;
  return *this;
}

Material::Builder& Material::Builder::FragmentCode(const uint32_t* code,
                                                   size_t code_size) {
  impl_->info.fragment.code = code;
  impl_->info.fragment.code_size = code_size;
  return *this;
}

Material::Builder& Material::Builder::Specialization(
    RenderAPI::ShaderStageFlags stage, uint32_t id, size_t size,
    const void* data) {
  std::vector<uint8_t>* data_vector;
  std::vector<RenderAPI::SpecializationMapEntry>* entries;
  if (stage == RenderAPI::ShaderStageFlagBits::kFragmentBit) {
    data_vector = &impl_->frag_specialization_data;
    entries = &impl_->frag_specialization_entries;
  } else if (stage == RenderAPI::ShaderStageFlagBits::kVertexBit) {
    data_vector = &impl_->vert_specialization_data;
    entries = &impl_->vert_specialization_entries;
  } else {
    return *this;
  }

  // Copy the data.
  const size_t offset = data_vector->size();
  data_vector->resize(offset + size);
  memcpy(data_vector->data() + offset, data, size);

  // Add the entry.
  RenderAPI::SpecializationMapEntry entry = {id, static_cast<uint32_t>(offset),
                                             size};
  entries->emplace_back(std::move(entry));

  return *this;
}

Material::Builder& Material::Builder::Blending(BlendMode mode) {
  impl_->blending = mode;
  return *this;
}
Material::Builder& Material::Builder::SetShading(Shading shading) {
  impl_->shading = shading;
  return *this;
}
Material::Builder& Material::Builder::CullMode(RenderAPI::CullModeFlags mode) {
  impl_->info.states.rasterization.cull_mode = mode;
  return *this;
}
Material::Builder& Material::Builder::DepthTest(bool value) {
  impl_->info.states.depth_stencil.depth_test_enable = value;
  return *this;
}
Material::Builder& Material::Builder::DepthWrite(bool value) {
  impl_->info.states.depth_stencil.depth_write_enable = value;
  return *this;
}
Material::Builder& Material::Builder::DepthClamp(bool value) {
  impl_->info.states.rasterization.depth_clamp_enable = value;
  return *this;
}
Material::Builder& Material::Builder::DepthCompareOp(RenderAPI::CompareOp op) {
  impl_->info.states.depth_stencil.depth_compare_op = op;
  return *this;
}
Material::Builder& Material::Builder::Viewport(RenderAPI::Viewport viewport) {
  impl_->info.states.viewport.viewports.emplace_back(std::move(viewport));
  return *this;
}
Material::Builder& Material::Builder::DynamicState(
    RenderAPI::DynamicState state) {
  impl_->info.states.dynamic_states.states.push_back(state);
  return *this;
}

// Inputs:
Material::Builder& Material::Builder::VertexAttribute(
    uint32_t location, uint32_t binding, RenderAPI::TextureFormat format,
    uint32_t offset) {
  impl_->info.vertex_input.attributes.emplace_back(location, binding, format,
                                                   offset);
  return *this;
}
Material::Builder& Material::Builder::VertexBinding(
    uint32_t binding, uint32_t stride, RenderAPI::VertexInputRate rate) {
  impl_->info.vertex_input.bindings.emplace_back(binding, stride, rate);
  return *this;
}

Material::Builder& Material::Builder::PushConstant(
    RenderAPI::ShaderStageFlags flags, uint32_t size, uint32_t offset) {
  if (offset == kOffsetNext) {
    offset = 0;
    for (const auto& it : impl_->layout_info.push_constants) {
      offset += it.size;
    }
  }
  impl_->layout_info.push_constants.emplace_back(flags, offset, size);
  return *this;
}

Material::Builder& Material::Builder::Uniform(
    uint32_t set, uint32_t binding, RenderAPI::ShaderStageFlags stages,
    size_t size, const void* default_data,
    RenderAPI::DescriptorBindingFlags flags) {
  ++impl_->num_uniform_buffers;
  if (impl_->descriptors.size() <= set) {
    impl_->descriptors.resize(set + 1);
  }
  auto& bindings = impl_->descriptors[set].bindings;

  // Create the uniform data and copy the data if any.
  DescriptorData uniform;
  uniform.type = RenderAPI::DescriptorType::kUniformBuffer;
  uniform.uniform.size = size;
  uniform.stages = stages;
  uniform.flags = flags;
  if (default_data) {
    uniform.uniform.data = std::make_unique<uint8_t[]>(size);
    memcpy(uniform.uniform.data.get(), default_data, size);
  }

  // Assign the uniform to the descriptor set..
  if (bindings.size() <= binding) {
    bindings.resize(binding + 1);
  }
  bindings[binding] = std::move(uniform);

  return *this;
}

Material::Builder& Material::Builder::Texture(
    uint32_t set, uint32_t binding, RenderAPI::ShaderStageFlags stages,
    const char* sampler, RenderAPI::DescriptorBindingFlags flags) {
  if (impl_->descriptors.size() <= set) {
    impl_->descriptors.resize(set + 1);
  }
  auto& bindings = impl_->descriptors[set].bindings;
  if (bindings.size() <= binding) {
    bindings.resize(binding + 1);
  }
  bindings[binding].type = RenderAPI::DescriptorType::kCombinedImageSampler;
  bindings[binding].stages = stages;
  bindings[binding].flags = flags;

  impl_->texture_to_sampler.emplace_back(set, binding, stages, sampler);

  return *this;
}

Material::Builder& Material::Builder::SetDescriptorFrequency(
    uint32_t set, DescriptorFrequency frequency) {
  if (impl_->descriptors.size() <= set) {
    impl_->descriptors.resize(set + 1);
  }
  impl_->descriptors[set].frequency = frequency;
  return *this;
}

Material::Builder& Material::Builder::Sampler(
    const char* name, RenderAPI::SamplerCreateInfo info) {
  impl_->samplers[name] = {std::move(info)};
  return *this;
}

Material* Material::Builder::Build() {
  assert(impl_->info.vertex.code);
  assert(impl_->info.fragment.code);

  impl_->info.fragment.module = RenderAPI::CreateShaderModule(
      impl_->device, impl_->info.fragment.code, impl_->info.fragment.code_size);
  impl_->info.vertex.module = RenderAPI::CreateShaderModule(
      impl_->device, impl_->info.vertex.code, impl_->info.vertex.code_size);
  impl_->info.vertex.code = nullptr;
  impl_->info.vertex.code_size = 0;
  impl_->info.fragment.code = nullptr;
  impl_->info.fragment.code_size = 0;

  impl_->info.states.blend.attachments.resize(1);
  if (impl_->blending == BlendMode::kBlend) {
    impl_->info.states.blend.attachments[0].blend_enable = true;
    impl_->info.states.blend.attachments[0].dst_alpha_blend_factor =
        RenderAPI::BlendFactor::kOneMinusSrcAlpha;
    impl_->info.states.blend.attachments[0].dst_color_blend_factor =
        RenderAPI::BlendFactor::kOneMinusSrcAlpha;
  } else if (impl_->blending == BlendMode::kMask) {
    assert(false);
  }

  // Create the samplers.
  std::vector<RenderAPI::Sampler> samplers;
  for (auto& it : impl_->samplers) {
    it.second.index = samplers.size();
    samplers.push_back(RenderAPI::CreateSampler(impl_->device, it.second.info));
  }

  // Map textures to samplers.
  size_t default_sampler = -1;
  for (auto& it : impl_->texture_to_sampler) {
    size_t sampler_index;
    if (it.sampler) {
      const auto& sampler_it = impl_->samplers.find(it.sampler);
      if (sampler_it == impl_->samplers.cend()) {
        std::cout << "Couldn't find sampler " << it.sampler << "\n";
        it.sampler = nullptr;
      } else {
        sampler_index = sampler_it->second.index;
      }
    }
    // Safety checking might leave us with a reset sampler, so we check this
    // anyway.
    if (!it.sampler) {
      if (default_sampler == -1) {
        // Create a default sampler.
        default_sampler = samplers.size();
        RenderAPI::SamplerCreateInfo info(
            RenderAPI::SamplerFilter::kLinear,
            RenderAPI::SamplerFilter::kLinear,
            RenderAPI::SamplerMipmapMode::kLinear,
            RenderAPI::SamplerAddressMode::kRepeat,
            RenderAPI::SamplerAddressMode::kRepeat,
            RenderAPI::SamplerAddressMode::kRepeat);
        samplers.push_back(RenderAPI::CreateSampler(impl_->device, info));
      }
      sampler_index = default_sampler;
    }

    // Set the uniform binding.
    auto& bindings = impl_->descriptors[it.set].bindings;
    bindings[it.binding].type =
        RenderAPI::DescriptorType::kCombinedImageSampler;
    bindings[it.binding].sampler = samplers[sampler_index];
    bindings[it.binding].stages = it.stages;
  }

  // Create the descriptor layout.
  for (auto& descriptor : impl_->descriptors) {
    RenderAPI::DescriptorSetLayoutCreateInfo descriptor_info;
    descriptor_info.bindings.resize(descriptor.bindings.size());
    uint32_t binding = 0;
    for (const auto& it : descriptor.bindings) {
      descriptor_info.bindings[binding].count = 1;
      descriptor_info.bindings[binding].stages = it.stages;
      descriptor_info.bindings[binding].type = it.type;
      descriptor_info.bindings[binding].flags = it.flags;
      ++binding;
    }
    descriptor.layout =
        RenderAPI::CreateDescriptorSetLayout(impl_->device, descriptor_info);
    impl_->layout_info.layouts.emplace_back(descriptor.layout);
  }

  // Create the pipeline layout.
  impl_->info.layout =
      RenderAPI::CreatePipelineLayout(impl_->device, impl_->layout_info);

  // Create the descriptor set pool.
  static constexpr uint32_t kNumBuffers = 3;
  static constexpr uint32_t kMaxInstances = 1000;
  static constexpr uint32_t kMaxBufferedInstances = kNumBuffers * kMaxInstances;
  RenderAPI::DescriptorSetPool descriptor_set_pool = RenderAPI::kInvalidHandle;
  RenderAPI::CreateDescriptorSetPoolCreateInfo pool_info;
  const uint32_t num_textures =
      static_cast<uint32_t>(impl_->texture_to_sampler.size());
  if (num_textures) {
    pool_info.pools.emplace_back(
        RenderAPI::DescriptorType::kCombinedImageSampler,
        num_textures * kMaxBufferedInstances);
  }
  if (impl_->num_uniform_buffers) {
    pool_info.pools.emplace_back(
        RenderAPI::DescriptorType::kUniformBuffer,
        impl_->num_uniform_buffers * kMaxBufferedInstances);
  }
  if (!pool_info.pools.empty()) {
    pool_info.max_sets = kMaxBufferedInstances;
    descriptor_set_pool =
        RenderAPI::CreateDescriptorSetPool(impl_->device, pool_info);
  }

  // Create the material.
  MaterialImpl* material = new MaterialImpl();
  material->device_ = impl_->device;
  material->pool_ = descriptor_set_pool;
  material->info_ = std::move(impl_->info);
  material->descriptors_ = std::move(impl_->descriptors);
  material->samplers_ = std::move(samplers);

  // Handle specialization.
  const size_t vert_specialization_size =
      impl_->vert_specialization_data.size();
  const size_t frag_specialization_size =
      impl_->frag_specialization_data.size();
  material->specialization_data_.resize(vert_specialization_size +
                                        frag_specialization_size);
  const size_t vert_specialization_offset = 0;
  const size_t frag_specialization_offset =
      impl_->vert_specialization_data.size();
  if (!impl_->vert_specialization_data.empty()) {
    memcpy(material->specialization_data_.data(),
           impl_->vert_specialization_data.data(), vert_specialization_size);
  }
  if (!impl_->frag_specialization_data.empty()) {
    memcpy(material->specialization_data_.data() + frag_specialization_offset,
           impl_->frag_specialization_data.data(), frag_specialization_size);
  }

  if (!impl_->vert_specialization_entries.empty()) {
    material->vert_entries = std::move(impl_->vert_specialization_entries);
    material->vert_specialization_.map_entries = material->vert_entries.data();
    material->vert_specialization_.map_entry_count =
        material->vert_entries.size();
    material->vert_specialization_.data_size = vert_specialization_size;
    material->vert_specialization_.data =
        material->specialization_data_.data() + vert_specialization_offset;
    material->info_.vertex.specialization = &material->vert_specialization_;
  }
  if (!impl_->frag_specialization_entries.empty()) {
    material->frag_entries = std::move(impl_->frag_specialization_entries);
    material->frag_specialization_.map_entries = material->frag_entries.data();
    material->frag_specialization_.map_entry_count =
        material->frag_entries.size();
    material->frag_specialization_.data_size = frag_specialization_size;
    material->frag_specialization_.data =
        material->specialization_data_.data() + frag_specialization_offset;
    material->info_.fragment.specialization = &material->frag_specialization_;
  }

  // Clean the builder for reuse.
  Material::BuilderDetails clean;
  clean.device = impl_->device;
  *impl_ = std::move(clean);

  return material;
}

Material::Builder::~Builder() noexcept {}

MaterialImpl::~MaterialImpl() {
  if (pool_ != RenderAPI::kInvalidHandle) {
    RenderAPI::DestroyDescriptorSetPool(pool_);
  }
  for (const auto& it : pipelines_) {
    RenderAPI::DestroyGraphicsPipeline(it.second);
  }
  for (const auto& it : samplers_) {
    RenderAPI::DestroySampler(device_, it);
  }
  for (const auto& it : descriptors_) {
    RenderAPI::DestroyDescriptorSetLayout(it.layout);
  }
  RenderAPI::DestroyPipelineLayout(device_, info_.layout);
  RenderAPI::DestroyShaderModule(device_, info_.fragment.module);
  RenderAPI::DestroyShaderModule(device_, info_.vertex.module);
}

RenderAPI::GraphicsPipeline MaterialImpl::GetPipeline(
    RenderAPI::RenderPass pass) {
  const auto& it = pipelines_.find(pass);
  if (it != pipelines_.cend()) {
    return it->second;
  }
  RenderAPI::GraphicsPipeline pipeline =
      RenderAPI::CreateGraphicsPipeline(device_, pass, info_);
  pipelines_[pass] = pipeline;
  return pipeline;
}

RenderAPI::PipelineLayout MaterialImpl::GetPipelineLayout() {
  return info_.layout;
}

MaterialInstance* MaterialImpl::CreateInstance() {
  // Create the descriptor sets.
  std::vector<MaterialDescriptor> descriptors(descriptors_.size());
  for (uint32_t descriptorIdx = 0; descriptorIdx < descriptors_.size();
       ++descriptorIdx) {
    if (descriptors_[descriptorIdx].frequency !=
        DescriptorFrequency::kMaterialInstance) {
      continue;
    }
    // Create param instances.
    const auto& src_set = descriptors_[descriptorIdx];
    descriptors[descriptorIdx].params.resize(src_set.bindings.size());
    auto& params = descriptors[descriptorIdx].params;
    for (uint32_t i = 0; i < src_set.bindings.size(); ++i) {
      params[i].type = src_set.bindings[i].type;
      size_t size = src_set.bindings[i].uniform.size;
      if (size) {
        params[i].buffers = RenderUtils::BufferedBuffer::Create(
            device_, RenderAPI::BufferUsageFlagBits::kUniformBuffer, size);
        params[i].data.size = size;
        params[i].data.data = std::make_unique<uint8_t[]>(size);
      } else {
        params[i].sampler = src_set.bindings[i].sampler;
      }
    }
    descriptors[descriptorIdx].set = RenderUtils::BufferedDescriptorSet::Create(
        device_, pool_, descriptors_[descriptorIdx].layout);
  }

  MaterialInstanceImpl* instance = new MaterialInstanceImpl();
  instance->device_ = device_;
  instance->material_ = this;
  instance->descriptors_ = std::move(descriptors);

  // Copy default uniform data.
  uint32_t setIdx = 0;
  for (const auto& set : descriptors_) {
    uint32_t bindingIdx = 0;
    for (const auto& binding : set.bindings) {
      if (binding.uniform.data) {
        instance->SetParam(setIdx, bindingIdx, binding.uniform.data.get());
      }
      ++bindingIdx;
    }
    ++setIdx;
  }
  instance->Commit();

  return instance;
}

MaterialParams* MaterialImpl::CreateParams(uint32_t set) const {
  MaterialParamsImpl* instance = new MaterialParamsImpl();
  instance->device_ = device_;
  instance->material_ = this;

  // Create param instances.
  const auto& src_set = descriptors_[set];
  auto& params = instance->params_;
  params.resize(src_set.bindings.size());
  for (uint32_t i = 0; i < src_set.bindings.size(); ++i) {
    params[i].type = src_set.bindings[i].type;
    size_t size = src_set.bindings[i].uniform.size;
    if (size) {
      params[i].buffers = RenderUtils::BufferedBuffer::Create(
          device_, RenderAPI::BufferUsageFlagBits::kUniformBuffer, size);
      params[i].data.size = size;
      params[i].data.data = std::make_unique<uint8_t[]>(size);
    } else {
      params[i].sampler = src_set.bindings[i].sampler;
    }
  }
  instance->set_ = RenderUtils::BufferedDescriptorSet::Create(
      device_, pool_, descriptors_[set].layout);

  // Copy default uniform data.
  uint32_t bindingIdx = 0;
  const auto& set_ref = descriptors_[set];
  for (const auto& binding : set_ref.bindings) {
    if (binding.uniform.data) {
      instance->SetParam(bindingIdx, binding.uniform.data.get());
    }
    ++bindingIdx;
  }
  instance->Commit();

  return instance;
}

// Forward base class functions to impl.
void Material::Destroy(Material* material) { delete upcast(material); }

MaterialInstance* Material::CreateInstance() {
  return upcast(this)->CreateInstance();
}
MaterialParams* Material::CreateParams(uint32_t set) const {
  return upcast(this)->CreateParams(set);
}
RenderAPI::GraphicsPipeline Material::GetPipeline(RenderAPI::RenderPass pass) {
  return upcast(this)->GetPipeline(pass);
}
RenderAPI::PipelineLayout Material::GetPipelineLayout() {
  return upcast(this)->GetPipelineLayout();
}