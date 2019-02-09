#pragma once

#include <RenderUtils/BufferedBuffer.h>
#include <RenderUtils/BufferedDescriptorSet.h>
#include <Renderer/Material.h>
#include <Renderer/MaterialParams.h>
#include "Material.h"

class MaterialParamsImpl : public MaterialParams {
 public:
  void SetTexture(uint32_t binding, RenderAPI::ImageView texture);
  void SetParam(uint32_t binding, const void* data);

  void Commit();

  const RenderAPI::DescriptorSet* DescriptorSet() const;

  ~MaterialParamsImpl();

 private:
  RenderAPI::Device device_;
  const Material* material_;
  struct MaterialParam;
  std::vector<MaterialParam> params_;
  RenderUtils::BufferedDescriptorSet set_;
  bool dirty_ = false;

  struct MaterialParam {
    RenderAPI::DescriptorType type;
    UniformData data;
    RenderUtils::BufferedBuffer buffers;
    RenderAPI::Sampler sampler;
    RenderAPI::ImageView texture = RenderAPI::kInvalidHandle;
    bool dirty = false;
  };
  friend class MaterialImpl;
};

IMPL_UPCAST(MaterialParams);