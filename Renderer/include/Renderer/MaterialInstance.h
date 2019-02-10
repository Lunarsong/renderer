#pragma once

#include <RenderAPI/RenderAPI.h>

class MaterialInstance {
 public:
  static void Destroy(MaterialInstance* instance);

  void SetTexture(uint32_t set, uint32_t binding, RenderAPI::ImageView texture);
  void SetParam(uint32_t set, uint32_t binding, const void* data);
  template <typename T>
  void SetParam(uint32_t set, uint32_t binding, const T& data);

  RenderAPI::ImageView GetTexture(uint32_t set, uint32_t binding);

  void Commit();

  const RenderAPI::DescriptorSet* DescriptorSet(uint32_t set) const;
};

template <typename T>
void MaterialInstance::SetParam(uint32_t set, uint32_t binding, const T& data) {
  SetParam(set, binding, reinterpret_cast<const void*>(&data));
}