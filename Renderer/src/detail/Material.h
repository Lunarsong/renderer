#pragma once

#include <RenderAPI/RenderAPI.h>
#include <Renderer/Material.h>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include "../upcast.h"

struct ParamData {
  size_t size = 0;
  std::unique_ptr<uint8_t[]> data;
};

struct DescriptorData {
  RenderAPI::DescriptorType type;
  RenderAPI::ShaderStageFlags stages;
  RenderAPI::DescriptorBindingFlags flags = 0;
  RenderAPI::Sampler sampler;
  ParamData uniform;
};
struct DescriptorBindings {
  std::vector<DescriptorData> bindings;
  RenderAPI::DescriptorSetLayout layout;
  DescriptorFrequency frequency = DescriptorFrequency::kMaterialInstance;
};

class MaterialImpl : public Material {
 public:
  ~MaterialImpl();

  RenderAPI::GraphicsPipeline GetPipeline(RenderAPI::RenderPass pass);
  RenderAPI::PipelineLayout GetPipelineLayout();

  MaterialInstance* CreateInstance();
  MaterialParams* CreateParams(uint32_t set) const;

 private:
  RenderAPI::Device device_;

  std::vector<RenderAPI::Sampler> samplers_;
  std::vector<DescriptorBindings> descriptors_;
  RenderAPI::GraphicsPipelineCreateInfo info_;
  std::unordered_map<RenderAPI::RenderPass, RenderAPI::GraphicsPipeline>
      pipelines_;
  RenderAPI::DescriptorSetPool pool_;

  // Shader specialization.
  RenderAPI::SpecializationInfo vert_specialization_;
  RenderAPI::SpecializationInfo frag_specialization_;
  std::vector<RenderAPI::SpecializationMapEntry> vert_entries;
  std::vector<RenderAPI::SpecializationMapEntry> frag_entries;
  std::vector<uint8_t> specialization_data_;

  friend class Material::Builder;
  MaterialImpl() = default;
};

IMPL_UPCAST(Material);