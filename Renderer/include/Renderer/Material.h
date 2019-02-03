#pragma once

#include <RenderAPI/RenderAPI.h>
#include <Renderer/BuilderBase.h>
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

enum class LightModel { kMetallicRoughess };

class Material {
  struct BuilderDetails;

 public:
  static void Destroy(Material* material);

  RenderAPI::GraphicsPipeline GetPipeline(RenderAPI::RenderPass pass);
  RenderAPI::PipelineLayout GetPipelineLayout();

 public:
  class Builder : public BuilderBase<BuilderDetails> {
   public:
    Builder(RenderAPI::Device device);

    // Shader:
    Builder& VertexCode(const uint32_t* code, size_t code_size);
    Builder& FragmentCode(const uint32_t* code, size_t code_size);

    // State:
    Builder& Blending(BlendMode mode);
    Builder& SetShading(Shading shading);
    Builder& CullMode(RenderAPI::CullModeFlags mode);
    Builder& DepthTest(bool value);
    Builder& DepthWrite(bool value);
    Builder& DepthClamp(bool value);
    Builder& Viewport(RenderAPI::Viewport viewport);

    // Inputs:
    Builder& AddVertexAttribute(VertexAttribute attribute);
    Builder& PushConstant(RenderAPI::ShaderStageFlags stages, uint32_t size,
                          uint32_t offset = kOffsetNext);
    Builder& Uniform(uint32_t binding, RenderAPI::ShaderStageFlags stages,
                     size_t size, const uint8_t* default_data = nullptr);
    Builder& Texture(uint32_t binding, RenderAPI::ShaderStageFlags stages,
                     const char* sampler = nullptr);
    Builder& Sampler(const char* name, RenderAPI::SamplerCreateInfo info =
                                           RenderAPI::SamplerCreateInfo());

    Material* Build();

    ~Builder() noexcept;
  };
};