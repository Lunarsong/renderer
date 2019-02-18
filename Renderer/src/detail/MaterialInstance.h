#pragma once

#include <RenderUtils/BufferedBuffer.h>
#include <RenderUtils/BufferedDescriptorSet.h>
#include <Renderer/Material.h>
#include <Renderer/MaterialInstance.h>
#include "Material.h"

struct MaterialParam {
  RenderAPI::DescriptorType type;
  ParamData data;
  RenderUtils::BufferedBuffer buffers;
  RenderAPI::Sampler sampler;
  RenderAPI::ImageView texture = 0;
  bool dirty = false;
};
struct MaterialDescriptor {
  std::vector<MaterialParam> params;
  RenderUtils::BufferedDescriptorSet set;
  bool dirty = false;
};

class MaterialInstanceImpl : public MaterialInstance {
 public:
  friend class MaterialImpl;

  void SetTexture(uint32_t set, uint32_t binding, RenderAPI::ImageView texture);
  void SetParam(uint32_t set, uint32_t binding, const void* data);
  void Commit();

  const RenderAPI::DescriptorSet* DescriptorSet(uint32_t set) const;
  Material* GetMaterial();

  ~MaterialInstanceImpl();

 private:
  RenderAPI::Device device_;
  Material* material_;
  std::vector<MaterialDescriptor> descriptors_;
  bool dirty_ = false;
};

IMPL_UPCAST(MaterialInstance);