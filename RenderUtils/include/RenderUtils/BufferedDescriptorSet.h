#pragma once

#include <RenderAPI/RenderAPI.h>

namespace RenderUtils {
class BufferedDescriptorSet {
 public:
  static BufferedDescriptorSet Create(RenderAPI::Device device,
                                      RenderAPI::DescriptorSetPool pool,
                                      RenderAPI::DescriptorSetLayout layout);

  RenderAPI::DescriptorSet operator++();
  operator RenderAPI::DescriptorSet&();
  operator const RenderAPI::DescriptorSet&() const;
  operator RenderAPI::DescriptorSet*();
  operator const RenderAPI::DescriptorSet*() const;

 private:
  static constexpr uint32_t kBufferCount = 3;

  RenderAPI::DescriptorSet sets_[kBufferCount];
  uint32_t current_ = 0;
};
}  // namespace RenderUtils