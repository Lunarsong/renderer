#include "detail/Material.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include "BuilderBase.h"
#include "detail/MaterialInstance.h"

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

RenderAPI::VertexAttributeType GetAttributeType(VertexAttribute attribute) {
  switch (attribute) {
    case VertexAttribute::kPosition:
      return RenderAPI::VertexAttributeType::kVec3;
    case VertexAttribute::kTexCoords:
      return RenderAPI::VertexAttributeType::kVec2;
    case VertexAttribute::kColor:
      return RenderAPI::VertexAttributeType::kVec3;
    case VertexAttribute::kNormals:
      return RenderAPI::VertexAttributeType::kVec3;
    case VertexAttribute::kBoneIndices:
      return RenderAPI::VertexAttributeType::kVec3;
    case VertexAttribute::kBoneWeights:
      return RenderAPI::VertexAttributeType::kVec3;
    default:
      throw "Unhandled attribute.";
  }
}
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
Material::Builder& Material::Builder::AddVertexAttribute(
    VertexAttribute attribute) {
  impl_->attributes.push_back(attribute);
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
    size_t size, const void* default_data) {
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
    const char* sampler) {
  if (impl_->descriptors.size() <= set) {
    impl_->descriptors.resize(set + 1);
  }
  auto& bindings = impl_->descriptors[set].bindings;
  if (bindings.size() <= binding) {
    bindings.resize(binding + 1);
  }
  bindings[binding].type = RenderAPI::DescriptorType::kCombinedImageSampler;
  bindings[binding].stages = stages;

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
    assert(false);
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
        samplers.push_back(RenderAPI::CreateSampler(impl_->device));
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
      ++binding;
    }
    descriptor.layout =
        RenderAPI::CreateDescriptorSetLayout(impl_->device, descriptor_info);
    impl_->layout_info.layouts.emplace_back(descriptor.layout);
  }

  // Create the pipeline layout.
  impl_->info.layout =
      RenderAPI::CreatePipelineLayout(impl_->device, impl_->layout_info);

  // Set the vertex layout.
  impl_->info.vertex_input.resize(1);
  impl_->info.vertex_input[0].layout.resize(impl_->attributes.size());
  size_t offset = 0;
  uint32_t binding = 0;
  for (const auto& vertex_attribute : impl_->attributes) {
    RenderAPI::VertexAttributeType attribute =
        GetAttributeType(vertex_attribute);
    impl_->info.vertex_input[0].layout[binding].type = attribute;
    impl_->info.vertex_input[0].layout[binding].offset = offset;
    offset += RenderAPI::GetVertexAttributeSize(attribute);
    ++binding;
  }

  // Create the descriptor set pool.
  RenderAPI::DescriptorSetPool descriptor_set_pool = RenderAPI::kInvalidHandle;
  RenderAPI::CreateDescriptorSetPoolCreateInfo pool_info;
  const uint32_t num_textures =
      static_cast<uint32_t>(impl_->texture_to_sampler.size());
  if (num_textures) {
    pool_info.pools.emplace_back(
        RenderAPI::DescriptorType::kCombinedImageSampler, num_textures * 3);
  }
  if (impl_->num_uniform_buffers) {
    pool_info.pools.emplace_back(RenderAPI::DescriptorType::kUniformBuffer,
                                 impl_->num_uniform_buffers * 3);
  }
  if (!pool_info.pools.empty()) {
    pool_info.max_sets = 3000;
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

MaterialInstance* MaterialImpl::CreateInstance() const {
  // Create the descriptor sets.
  std::vector<MaterialDescriptor> descriptors(descriptors_.size());
  for (uint32_t descriptorIdx = 0; descriptorIdx < descriptors_.size();
       ++descriptorIdx) {
    if (descriptors_[descriptorIdx].frequency !=
        DescriptorFrequency::kPerMaterialInstance) {
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
  instance->material_ = this;
  instance->descriptors_ = std::move(descriptors);

  // Copy default uniform data.
  uint32_t setIdx = 0;
  uint32_t bindingIdx = 0;
  for (const auto& set : descriptors_) {
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

// Forward base class functions to impl.
void Material::Destroy(Material* material) { delete upcast(material); }

MaterialInstance* Material::CreateInstance() const {
  return upcast(this)->CreateInstance();
}
RenderAPI::GraphicsPipeline Material::GetPipeline(RenderAPI::RenderPass pass) {
  return upcast(this)->GetPipeline(pass);
}
RenderAPI::PipelineLayout Material::GetPipelineLayout() {
  return upcast(this)->GetPipelineLayout();
}