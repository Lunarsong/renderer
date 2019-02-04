#pragma once

#include <RenderAPI/RenderAPI.h>

namespace RenderUtils {
class BufferedBuffer {
 public:
  static BufferedBuffer Create(
      RenderAPI::Device device, RenderAPI::BufferUsageFlags usage, size_t size,
      RenderAPI::MemoryUsage memory_usage = RenderAPI::MemoryUsage::kCpuToGpu);

  BufferedBuffer();
  void Destroy();

  RenderAPI::Buffer operator++();
  operator RenderAPI::Buffer&();
  operator const RenderAPI::Buffer&() const;
  operator RenderAPI::Buffer*();
  operator const RenderAPI::Buffer*() const;
  operator const bool() const;

 private:
  static constexpr uint32_t kBufferCount = 3;

  RenderAPI::Buffer buffers_[kBufferCount];
  uint32_t current_ = 0;
};
}  // namespace RenderUtils