#include <RenderUtils/BufferedBuffer.h>

namespace RenderUtils {
BufferedBuffer BufferedBuffer::Create(RenderAPI::Device device,
                                      RenderAPI::BufferUsageFlags usage,
                                      size_t size,
                                      RenderAPI::MemoryUsage memory_usage) {
  BufferedBuffer buffer;
  for (int i = 0; i < BufferedBuffer::kBufferCount; ++i) {
    buffer.buffers_[i] =
        RenderAPI::CreateBuffer(device, usage, size, memory_usage);
  }
  return buffer;
}

BufferedBuffer::BufferedBuffer() {
  for (int i = 0; i < BufferedBuffer::kBufferCount; ++i) {
    buffers_[i] = RenderAPI::kInvalidHandle;
  }
}
void BufferedBuffer::Destroy() {
  for (int i = 0; i < BufferedBuffer::kBufferCount; ++i) {
    RenderAPI::DestroyBuffer(buffers_[i]);
  }
}

RenderAPI::DescriptorSet BufferedBuffer::operator++() {
  current_ = (current_ + 1) % BufferedBuffer::kBufferCount;
  return buffers_[current_];
}

BufferedBuffer::operator RenderAPI::DescriptorSet&() {
  return buffers_[current_];
}

BufferedBuffer::operator const RenderAPI::DescriptorSet&() const {
  return buffers_[current_];
}

BufferedBuffer::operator RenderAPI::DescriptorSet*() {
  return &buffers_[current_];
}

BufferedBuffer::operator const RenderAPI::DescriptorSet*() const {
  return &buffers_[current_];
}

BufferedBuffer::operator const bool() const {
  return buffers_[current_] != RenderAPI::kInvalidHandle;
}

}  // namespace RenderUtils
