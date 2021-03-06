#pragma once

#include <RenderAPI/RenderAPI.h>

class Material;
class MaterialInstance {
 public:
  static void Destroy(MaterialInstance* instance);

  void SetTexture(uint32_t set, uint32_t binding, RenderAPI::ImageView texture);
  void SetParam(uint32_t set, uint32_t binding, const void* data);
  template <typename T>
  void SetParam(uint32_t set, uint32_t binding, const T& data);

  void Commit();

  const RenderAPI::DescriptorSet* DescriptorSet(uint32_t set) const;
  Material* GetMaterial();
};

template <typename T>
void MaterialInstance::SetParam(uint32_t set, uint32_t binding, const T& data) {
  SetParam(set, binding, reinterpret_cast<const void*>(&data));
}