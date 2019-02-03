#include "detail/Material.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include "BuilderBase.h"

namespace {
struct SamplerInfo {
  RenderAPI::SamplerCreateInfo info;
  uint32_t index;
};

struct TextureInfo {
  const char* sampler;
  RenderAPI::ShaderStageFlags stages;
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
  std::vector<UniformBinding> uniforms;
  std::unordered_map<std::string, SamplerInfo> samplers;
  std::unordered_map<uint32_t, TextureInfo> texture_to_sampler;
  std::vector<VertexAttribute> attributes;
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
Material::Builder& Material::Builder::Viewport(RenderAPI::Viewport viewport) {
  impl_->info.states.viewport.viewports.emplace_back(std::move(viewport));
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
    uint32_t binding, RenderAPI::ShaderStageFlags stages, size_t size,
    const uint8_t* default_data) {
  // Create the uniform data and copy the data if any.
  UniformBinding uniform;
  uniform.type = RenderAPI::DescriptorType::kUniformBuffer;
  uniform.uniform.size = size;
  uniform.stages = stages;
  if (default_data) {
    uniform.uniform.data = std::make_unique<uint8_t[]>(size);
    memcpy(uniform.uniform.data.get(), default_data, size);
  }

  // Assign the uniform to the descriptor set..
  impl_->uniforms[binding] = std::move(uniform);

  return *this;
}

Material::Builder& Material::Builder::Texture(
    uint32_t binding, RenderAPI::ShaderStageFlags stages, const char* sampler) {
  impl_->texture_to_sampler[binding] = {sampler, stages};
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
    if (it.second.sampler) {
      const auto& sampler_it = impl_->samplers.find(it.second.sampler);
      if (sampler_it == impl_->samplers.cend()) {
        std::cout << "Couldn't find sampler " << it.second.sampler << "\n";
        it.second.sampler = nullptr;
      } else {
        sampler_index = sampler_it->second.index;
      }
    }
    // Safety checking might leave us with a reset sampler, so we check this
    // anyway.
    if (!it.second.sampler) {
      if (default_sampler == -1) {
        // Create a default sampler.
        default_sampler = samplers.size();
        samplers.push_back(RenderAPI::CreateSampler(impl_->device));
      }
      sampler_index = default_sampler;
      impl_->uniforms[it.first].type =
          RenderAPI::DescriptorType::kCombinedImageSampler;
      impl_->uniforms[it.first].sampler = samplers[sampler_index];
      impl_->uniforms[it.first].stages = it.second.stages;
    }
  }

  // Create the descriptor layout.
  RenderAPI::DescriptorSetLayoutCreateInfo descriptor_info;
  descriptor_info.bindings.reserve(impl_->uniforms.size());
  uint32_t binding = 0;
  for (const auto& it : impl_->uniforms) {
    descriptor_info.bindings[binding].count = 1;
    descriptor_info.bindings[binding].stages = impl_->uniforms[binding].stages;
    descriptor_info.bindings[binding].type = impl_->uniforms[binding].type;
    ++binding;
  }
  RenderAPI::DescriptorSetLayout descriptor_layout =
      RenderAPI::CreateDescriptorSetLayout(impl_->device, descriptor_info);

  // Create the pipeline layout.
  impl_->layout_info.layouts.emplace_back(descriptor_layout);
  impl_->info.layout =
      RenderAPI::CreatePipelineLayout(impl_->device, impl_->layout_info);

  // Set the vertex layout.
  impl_->info.vertex_input.resize(1);
  impl_->info.vertex_input[0].layout.resize(impl_->attributes.size());
  size_t offset = 0;
  binding = 0;
  for (const auto& vertex_attribute : impl_->attributes) {
    RenderAPI::VertexAttributeType attribute =
        GetAttributeType(vertex_attribute);
    impl_->info.vertex_input[0].layout[binding].type = attribute;
    impl_->info.vertex_input[0].layout[binding].offset = offset;
    offset += RenderAPI::GetVertexAttributeSize(attribute);
    ++binding;
  }

  MaterialImpl* material = new MaterialImpl();
  material->device_ = impl_->device;
  material->descriptor_layout_ = descriptor_layout;
  material->info_ = std::move(impl_->info);
  material->bindings_ = std::move(impl_->uniforms);
  material->samplers_ = std::move(samplers);

  return material;
}

Material::Builder::~Builder() noexcept {}

MaterialImpl::~MaterialImpl() {
  for (const auto& it : pipelines_) {
    RenderAPI::DestroyGraphicsPipeline(it.second);
  }
  for (const auto& it : samplers_) {
    RenderAPI::DestroySampler(device_, it);
  }
  RenderAPI::DestroyPipelineLayout(device_, info_.layout);
  RenderAPI::DestroyDescriptorSetLayout(descriptor_layout_);
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

// Forward base class functions to impl.
void Material::Destroy(Material* material) { delete upcast(material); }

RenderAPI::GraphicsPipeline Material::GetPipeline(RenderAPI::RenderPass pass) {
  return upcast(this)->GetPipeline(pass);
}
RenderAPI::PipelineLayout Material::GetPipelineLayout() {
  return upcast(this)->GetPipelineLayout();
}