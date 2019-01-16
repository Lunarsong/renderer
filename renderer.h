#pragma once

#include <cstdint>
#include <vector>
#include "renderer_texture.h"

namespace Renderer {
using HandleType = uint64_t;
static constexpr HandleType kInvalidHandle = ((HandleType)-1);

using Instance = HandleType;
using Device = HandleType;
using SwapChain = HandleType;
using GraphicsPipeline = HandleType;
using PipelineLayout = HandleType;
using RenderPass = HandleType;
using Framebuffer = HandleType;
using Buffer = HandleType;
using CommandPool = HandleType;
using CommandBuffer = HandleType;
using Semaphore = HandleType;
using Fence = HandleType;
using DescriptorSetLayout = HandleType;
using DescriptorSetPool = HandleType;
using DescriptorSet = HandleType;
using Image = HandleType;
using ImageView = HandleType;

enum class VertexAttributeType { kFloat, kVec2, kVec3, kVec4 };

struct VertexAttribute {
  VertexAttributeType type;
  uint32_t offset;
};
using VertexLayout = std::vector<VertexAttribute>;
struct VertexInputBinding {
  VertexLayout layout;
};
using VertexInputBindings = std::vector<VertexInputBinding>;

// Instance.
Instance Create(const char* const* extensions = nullptr,
                uint32_t extensions_count = 0);
void Destroy(Instance instance);

// Device.
Device CreateDevice(Instance instance);
void DestroyDevice(Device device);
void DeviceWaitIdle(Device device);

// Swapchain.
SwapChain CreateSwapChain(Device device, uint32_t width, uint32_t height);
void DestroySwapChain(SwapChain swapchain);
uint32_t GetSwapChainLength(SwapChain swapchain);
void AcquireNextImage(SwapChain swapchain, uint64_t timeout_ns,
                      Semaphore semaphore, uint32_t* out_image_index);

// Graphics Pipeline.
struct ShaderCreateInfo {
  size_t code_size = 0;
  const uint32_t* code = nullptr;
};

PipelineLayout CreatePipelineLayout(
    Device device, const std::vector<DescriptorSetLayout>& layout);
void DestroyPipelineLayout(Device device, PipelineLayout layout);

struct GraphicsPipelineCreateInfo {
  ShaderCreateInfo vertex;
  ShaderCreateInfo fragment;
  VertexInputBindings vertex_input;
  PipelineLayout layout;
};
GraphicsPipeline CreateGraphicsPipeline(Device device, RenderPass pass,
                                        const GraphicsPipelineCreateInfo& info);
void DestroyGraphicsPipeline(GraphicsPipeline pipeline);

// Render pass.
RenderPass CreateRenderPass(Device device, SwapChain swapchain);
void DestroyRenderPass(RenderPass pass);

// Framebuffers.
Framebuffer CreateSwapChainFramebuffer(SwapChain swapchain, uint32_t index,
                                       RenderPass pass);
void DestroyFramebuffer(Framebuffer buffer);

// Buffers.
enum class MemoryUsage { kGpu, kCpu, kCpuToGpu, kGpuToCpu };
enum class BufferType { kVertex, kIndex, kUniform };
Buffer CreateBuffer(Device device, BufferType type, uint64_t size,
                    MemoryUsage memory_usage = MemoryUsage::kCpuToGpu);
void DestroyBuffer(Buffer buffer);
void* MapBuffer(Buffer buffer);
void UnmapBuffer(Buffer buffer);

// Command pools.
CommandPool CreateCommandPool(Device device);
void DestroyCommandPool(CommandPool pool);

// Command Buffers.
CommandBuffer CreateCommandBuffer(CommandPool pool);
void DestroyCommandBuffer(CommandBuffer buffer);
void CmdBegin(CommandBuffer buffer);
void CmdEnd(CommandBuffer buffer);
void CmdBeginRenderPass(CommandBuffer buffer, RenderPass pass,
                        Framebuffer framebuffer);
void CmdEndRenderPass(CommandBuffer buffer);
void CmdBindPipeline(CommandBuffer buffer, GraphicsPipeline pipeline);
void CmdBindVertexBuffers(CommandBuffer buffer, uint32_t first_binding,
                          uint32_t binding_count, const Buffer* buffers,
                          const uint64_t* offsets = nullptr);
void CmdBindDescriptorSets(CommandBuffer cmd, int type, PipelineLayout layout,
                           uint32_t first, uint32_t count,
                           const DescriptorSet* sets,
                           uint32_t dynamic_sets_count = 0,
                           const uint32_t* dynamic_sets_offsets = nullptr);

enum class IndexType { kUInt16 = 0, kUInt32 = 1 };
void CmdBindIndexBuffer(CommandBuffer cmd, Buffer buffer, IndexType type,
                        uint64_t offset = 0);
void CmdDraw(CommandBuffer buffer, uint32_t vertex_count,
             uint32_t instance_count = 1, uint32_t first_vertex = 0,
             uint32_t first_instance = 0);
void CmdDrawIndexed(CommandBuffer cmd, uint32_t index_count,
                    uint32_t instance_count = 1, uint32_t first_index = 0,
                    int32_t vertex_offset = 0, uint32_t first_instance = 0);
struct BufferCopy {
  uint64_t src_offset = 0;
  uint64_t dst_offset = 0;
  uint64_t size;
};
void CmdCopyBuffer(CommandBuffer cmd, Buffer src, Buffer dst,
                   uint32_t region_count, const BufferCopy* regions);

// Semaphores.
Semaphore CreateSemaphore(Device);
void DestroySemaphore(Semaphore);

// Fences.
Fence CreateFence(Device device, bool signaled = false);
void DestroyFence(Fence fence);
void WaitForFences(const Fence* fences, uint32_t count, bool wait_for_all,
                   uint64_t timeout_ns);
void ResetFences(const Fence* fences, uint32_t count);

// Descriptor sets.
enum class DescriptorType {
  kSampler = 0,
  kCombinedImageSampler = 1,
  kSampledImage = 2,
  kStorageImage = 3,
  kUniformTexelBuffer = 4,
  kStorageTexelBuffer = 5,
  kUniformBuffer = 6,
  kStorageBuffer = 7,
  kUniformBufferDynamic = 8,
  kStorageBufferDynamic = 9,
  kInputAttachment = 10,
  kInlineUniformBlockExt = 1000138000,
  kAccelerationStructureNV = 1000165000,
};
enum ShaderStageFlagBits {
  kVertexBit = 0x00000001,
  kTessellationControlBit = 0x00000002,
  kTessellationEvaluationBit = 0x00000004,
  kGeometryBit = 0x00000008,
  kFragmentBit = 0x00000010,
  kComputeBit = 0x00000020,
  kAllGraphics = 0x0000001F,
  kAll = 0x7FFFFFFF,
  kRayGenBitNV = 0x00000100,
  kAnyHitBitNV = 0x00000200,
  kClosestHitBitNV = 0x00000400,
  kMissBitNV = 0x00000800,
  kIntersectionBitNV = 0x00001000,
  kCallableBitNV = 0x00002000,
  kTaskBitNV = 0x00000040,
  kMeshBitNV = 0x00000080,
};
struct DescriptorSetLayoutBinding {
  DescriptorType type;
  uint32_t count = 0;
  ShaderStageFlagBits stages;
};
struct DescriptorSetLayoutCreateInfo {
  std::vector<DescriptorSetLayoutBinding> bindings;
};

DescriptorSetLayout CreateDescriptorSetLayout(
    Device device, const DescriptorSetLayoutCreateInfo& info);
void DestroyDescriptorSetLayout(DescriptorSetLayout layout);

// Descriptor Set Pool.
struct DescriptorPoolSize {
  DescriptorType type;
  uint32_t count;
};
struct CreateDescriptorSetPoolCreateInfo {
  std::vector<DescriptorPoolSize> pools;
  uint32_t max_sets;
};
DescriptorSetPool CreateDescriptorSetPool(
    Device device, const CreateDescriptorSetPoolCreateInfo& info);
void DestroyDescriptorSetPool(DescriptorSetPool pool);

// Descriptor Sets.
void AllocateDescriptorSets(DescriptorSetPool pool,
                            const std::vector<DescriptorSetLayout>& layouts,
                            DescriptorSet* sets);
struct DescriptorImageInfo {};
struct DescriptorBufferInfo {
  Buffer buffer;
  uint64_t offset = 0;
  uint64_t range;
};
struct WriteDescriptorSet {
  DescriptorSet set;
  uint32_t binding;
  uint32_t dst_array_element = 0;
  uint32_t descriptor_count = 0;
  DescriptorType type;
  const DescriptorImageInfo* images = nullptr;
  const DescriptorBufferInfo* buffers = nullptr;
};
void UpdateDescriptorSets(Device device, uint32_t descriptor_write_count,
                          WriteDescriptorSet* descriptor_writes,
                          uint32_t descriptor_copy_count = 0,
                          void* descriptor_copies = nullptr);

// Queues.
struct SubmitInfo {
  const Semaphore* wait_semaphores;
  uint32_t wait_semaphores_count = 0;
  uint32_t wait_flags;
  const Semaphore* signal_semaphores;
  uint32_t signal_semaphores_count = 0;
  const CommandBuffer* command_buffers;
  uint32_t command_buffers_count;
};
void QueueSubmit(Device device, const SubmitInfo& info,
                 Fence fence = kInvalidHandle);

struct PresentInfo {
  uint32_t wait_semaphores_count = 0;
  const Semaphore* wait_semaphores;
};
void QueuePresent(SwapChain swapchain, uint32_t image, const PresentInfo& info);

// Images.
Image CreateImage(Device device, const ImageCreateInfo& info);
void DestroyImage(Image image);

// Helpers.
void StageCopyDataToBuffer(CommandPool pool, Buffer buffer, const void* data,
                           uint64_t size);
struct BufferImageCopy {
  Offset3D offset = {0, 0, 0};
  Extent3D extent;
  BufferImageCopy() = default;
  BufferImageCopy(uint32_t x, uint32_t y, uint32_t z, uint32_t width,
                  uint32_t height, uint32_t depth)
      : offset(x, y, z), extent(width, height, depth) {}
};
void StageCopyDataToImage(CommandPool pool, Image image, const void* data,
                          uint64_t size, const BufferImageCopy& info);

}  // namespace Renderer