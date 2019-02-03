#pragma once

#include <RenderAPI/RenderAPI.h>
#include <Renderer/Material.h>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include "../upcast.h"

struct UniformData {
  size_t size;
  std::unique_ptr<uint8_t[]> data;
};

struct UniformBinding {
  RenderAPI::DescriptorType type;
  RenderAPI::ShaderStageFlags stages;
  RenderAPI::Sampler sampler;
  UniformData uniform;
};

class MaterialImpl : public Material {
 public:
  ~MaterialImpl();

  RenderAPI::GraphicsPipeline GetPipeline(RenderAPI::RenderPass pass);
  RenderAPI::PipelineLayout GetPipelineLayout();

 private:
  RenderAPI::Device device_;
  RenderAPI::DescriptorSetLayout descriptor_layout_;

  std::vector<RenderAPI::Sampler> samplers_;
  std::vector<UniformBinding> bindings_;
  RenderAPI::GraphicsPipelineCreateInfo info_;
  std::unordered_map<RenderAPI::RenderPass, RenderAPI::GraphicsPipeline>
      pipelines_;

  friend class Material::Builder;
  MaterialImpl() = default;
};

IMPL_UPCAST(Material);