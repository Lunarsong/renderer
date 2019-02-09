#pragma once

#include <RenderAPI/RenderAPI.h>

class MaterialParams {
 public:
  static void Destroy(MaterialParams* instance);

  void SetTexture(uint32_t binding, RenderAPI::ImageView texture);
  void SetParam(uint32_t binding, const void* data);
  template <typename T>
  void SetParam(uint32_t binding, const T& data);

  void Commit();

  const RenderAPI::DescriptorSet* DescriptorSet() const;
};

template <typename T>
void MaterialParams::SetParam(uint32_t binding, const T& data) {
  SetParam(binding, reinterpret_cast<const void*>(&data));
}