#pragma once

#include <RenderAPI/RenderAPI.h>
#include <Renderer/BuilderBase.h>
#include <Renderer/MaterialInstance.h>
#include <Renderer/MaterialParams.h>
#include <cstdint>

constexpr uint32_t kOffsetNext = -1;

enum class VertexAttribute {
  kPosition,
  kTexCoords,
  kColor,
  kNormals,
  kBoneIndices,
  kBoneWeights,
};

enum class Shading {
  kUnlit,
  kLit,
};

enum class BlendMode {
  kOpaque,
  kMask,
  kBlend,
};

enum class DescriptorFrequency {
  kMaterialInstance = 0,
  kUndefined = 1,
};

enum class LightModel { kMetallicRoughess };

class Material {
  struct BuilderDetails;

 public:
  static void Destroy(Material* material);

  MaterialInstance* CreateInstance();
  MaterialParams* CreateParams(uint32_t set) const;

  RenderAPI::GraphicsPipeline GetPipeline(RenderAPI::RenderPass pass);
  RenderAPI::PipelineLayout GetPipelineLayout();

 public:
  class Builder : public BuilderBase<BuilderDetails> {
   public:
    Builder(RenderAPI::Device device);

    // Shader:
    Builder& VertexCode(const uint32_t* code, size_t code_size);
    Builder& FragmentCode(const uint32_t* code, size_t code_size);
    Builder& Specialization(RenderAPI::ShaderStageFlags stage, uint32_t id,
                            size_t size, const void* data);
    template <typename T>
    Builder& Specialization(RenderAPI::ShaderStageFlags stage, uint32_t id,
                            const T& data);

    // State:
    Builder& Blending(BlendMode mode);
    Builder& SetShading(Shading shading);
    Builder& CullMode(RenderAPI::CullModeFlags mode);
    Builder& DepthTest(bool value);
    Builder& DepthWrite(bool value);
    Builder& DepthClamp(bool value);
    Builder& DepthCompareOp(RenderAPI::CompareOp op);
    Builder& Viewport(RenderAPI::Viewport viewport);
    Builder& DynamicState(RenderAPI::DynamicState state);

    // Inputs:
    Builder& VertexAttribute(uint32_t location, uint32_t binding,
                             RenderAPI::TextureFormat format, uint32_t offset);
    Builder& VertexBinding(
        uint32_t binding, uint32_t stride,
        RenderAPI::VertexInputRate rate = RenderAPI::VertexInputRate::kVertex);
    Builder& PushConstant(RenderAPI::ShaderStageFlags stages, uint32_t size,
                          uint32_t offset = kOffsetNext);
    Builder& Uniform(uint32_t set, uint32_t binding,
                     RenderAPI::ShaderStageFlags stages, size_t size,
                     const void* default_data = nullptr,
                     RenderAPI::DescriptorBindingFlags flags = 0);
    Builder& Texture(uint32_t set, uint32_t binding,
                     RenderAPI::ShaderStageFlags stages,
                     const char* sampler = nullptr,
                     RenderAPI::DescriptorBindingFlags flags = 0);
    Builder& SetDescriptorFrequency(uint32_t set,
                                    DescriptorFrequency frequency);
    Builder& Sampler(const char* name, RenderAPI::SamplerCreateInfo info =
                                           RenderAPI::SamplerCreateInfo());

    Material* Build();

    ~Builder() noexcept;
  };
};

template <typename T>
Material::Builder& Material::Builder::Specialization(
    RenderAPI::ShaderStageFlags stage, uint32_t id, const T& data) {
  return Specialization(stage, id, sizeof(T), &data);
}