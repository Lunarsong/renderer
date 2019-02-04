#pragma once

#include <cstdint>
#include <initializer_list>
#include <vector>
#include "RenderAPI_pipelines.h"
#include "RenderAPI_texture.h"

namespace RenderAPI {
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
using Sampler = HandleType;
using ShaderModule = HandleType;

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
TextureFormat GetSwapChainImageFormat(SwapChain swapchain);
ImageView GetSwapChainImageView(SwapChain swapchain, uint32_t index);

// Graphics Pipeline.
struct ShaderCreateInfo {
  size_t code_size = 0;
  const uint32_t* code = nullptr;
  ShaderModule module = 0;
};

struct PipelineLayoutCreateInfo {
  std::vector<DescriptorSetLayout> layouts;
  std::vector<PushConstantRange> push_constants;
};
PipelineLayout CreatePipelineLayout(Device device,
                                    const PipelineLayoutCreateInfo& info);
void DestroyPipelineLayout(Device device, PipelineLayout layout);

struct GraphicsPipelineCreateInfo {
  ShaderCreateInfo vertex;
  ShaderCreateInfo fragment;
  VertexInputBindings vertex_input;
  PipelineLayout layout;
  GraphicsPipelineStateInfo states;
};
GraphicsPipeline CreateGraphicsPipeline(Device device, RenderPass pass,
                                        const GraphicsPipelineCreateInfo& info);
void DestroyGraphicsPipeline(GraphicsPipeline pipeline);

enum AttachmentDescriptionFlags {

};

enum SampleCountFlagBits {
  SAMPLE_COUNT_1_BIT = 0x00000001,
  SAMPLE_COUNT_2_BIT = 0x00000002,
  SAMPLE_COUNT_4_BIT = 0x00000004,
  SAMPLE_COUNT_8_BIT = 0x00000008,
  SAMPLE_COUNT_16_BIT = 0x00000010,
  SAMPLE_COUNT_32_BIT = 0x00000020,
  SAMPLE_COUNT_64_BIT = 0x00000040,
};

enum class AttachmentLoadOp {
  kLoad = 0,
  kClear = 1,
  kDontCare = 2,
};

enum class AttachmentStoreOp {
  kStore = 0,
  kDontCare = 1,
};

enum class ImageLayout {
  kUndefined = 0,
  kGeneral = 1,
  kColorAttachmentOptimal = 2,
  kDepthStencilAttachmentOptimal = 3,
  kDepthStencilReadOnlyOptimal = 4,
  kShaderReadOnlyOptimal = 5,
  kTransferSrcOptimal = 6,
  kTransferDrcOptimal = 7,
  kPreinitialized = 8,
  kDepthReadOnlyStencilAttachmentOptimal = 1000117000,
  kDepthAttachmentStencilReadonlyOptimal = 1000117001,
  kPresentSrcKHR = 1000001002,
  kSharedPresentKHR = 1000111000,
  kShadingRateOptimalNV = 1000164003,
  kFragmentDensityMapOptimalEXT = 1000218000,
};

struct AttachmentDescription {
  uint32_t flags = 0;
  TextureFormat format = TextureFormat::kUndefined;
  SampleCountFlagBits samples = SAMPLE_COUNT_1_BIT;
  AttachmentLoadOp load_op = AttachmentLoadOp::kDontCare;
  AttachmentStoreOp store_op = AttachmentStoreOp::kDontCare;
  AttachmentLoadOp stencil_load_op = AttachmentLoadOp::kDontCare;
  AttachmentStoreOp stencil_store_op = AttachmentStoreOp::kDontCare;
  ImageLayout initial_layout = ImageLayout::kUndefined;
  ImageLayout final_layout;
};

struct RenderPassCreateInfo {
  std::vector<AttachmentDescription> attachments;
};

// Render pass.
RenderPass CreateRenderPass(Device device, const RenderPassCreateInfo& info);
void DestroyRenderPass(RenderPass pass);

// Framebuffers.
struct FramebufferCreateInfo {
  RenderPass pass;
  std::vector<ImageView> attachments;
  uint32_t width;
  uint32_t height;
};
Framebuffer CreateFramebuffer(Device device, const FramebufferCreateInfo& info);
void DestroyFramebuffer(Framebuffer buffer);

// Buffers.
namespace BufferUsageFlagBits {
enum BufferUsageFlagBits {
  kTransferSrc = 0x00000001,
  kTransferDst = 0x00000002,
  kUniformTexelBuffer = 0x00000004,
  kStorageTexelBuffer = 0x00000008,
  kUniformBuffer = 0x00000010,
  kStorageBuffer = 0x00000020,
  kIndexBuffer = 0x00000040,
  kVertexBuffer = 0x00000080,
  kIndirectBuffer = 0x00000100,
  kTransformFeedbackBufferEXT = 0x00000800,
  kTransformFeedbackCounterBufferEXT = 0x00001000,
  kConditionalRenderingEXT = 0x00000200,
  kRayTracingNV = 0x00000400,
  kShaderDeviceAddressEXT = 0x00020000,
};
}  // namespace BufferUsageFlagBits
using BufferUsageFlags = uint32_t;

enum class MemoryUsage { kGpu, kCpu, kCpuToGpu, kGpuToCpu };
Buffer CreateBuffer(Device device, BufferUsageFlags usage, uint64_t size,
                    MemoryUsage memory_usage = MemoryUsage::kCpuToGpu);
void DestroyBuffer(Buffer buffer);
void* MapBuffer(Buffer buffer);
void UnmapBuffer(Buffer buffer);

// Command pools.
enum class CommandPoolCreateFlags {
  kDefault = 0,
  kTransient = 0x00000001,
  kResetCommand = 0x00000002,
  kProtected = 0x00000004
};
CommandPool CreateCommandPool(
    Device device,
    CommandPoolCreateFlags flags = CommandPoolCreateFlags::kDefault);
void DestroyCommandPool(CommandPool pool);

struct ColorClearValue {
  union {
    struct {
      float r;
      float g;
      float b;
      float a;
    };
    float rgba[4];
  };
  ColorClearValue(float r = 0.0f, float g = 0.0f, float b = 0.0f,
                  float a = 1.0f)
      : r(r), g(g), b(b), a(a) {}
};
struct DepthStencilClearValue {
  float depth = 1.0f;
  uint32_t stencil = 0;
};
struct ClearValue {
  union {
    ColorClearValue color;
    DepthStencilClearValue depth_stencil;
  };
  ClearValue(ColorClearValue color = ColorClearValue())
      : color(std::move(color)) {}
  ClearValue(DepthStencilClearValue depth_stencil)
      : depth_stencil(std::move(depth_stencil)) {}
};
// Command Buffers.
struct BeginRenderPassInfo {
  RenderPass pass;
  Framebuffer framebuffer;
  Rect2D render_area;
  uint32_t clear_values_count = 0;
  const ClearValue* clear_values = nullptr;

  BeginRenderPassInfo(RenderPass pass, Framebuffer framebuffer,
                      Rect2D render_area, uint32_t clear_values_count = 0,
                      const ClearValue* clear_values = nullptr)
      : pass(pass),
        framebuffer(framebuffer),
        render_area(render_area),
        clear_values_count(clear_values_count),
        clear_values(clear_values) {}
};
CommandBuffer CreateCommandBuffer(CommandPool pool);
void DestroyCommandBuffer(CommandBuffer buffer);
void CmdBegin(CommandBuffer buffer);
void CmdEnd(CommandBuffer buffer);
void CmdSetViewport(CommandBuffer buffer, uint32_t first_viewport,
                    uint32_t viewport_count, const Viewport* viewports);
void CmdSetScissor(CommandBuffer buffer, uint32_t first_scissor, uint32_t count,
                   const Rect2D* scissors);
void CmdBeginRenderPass(CommandBuffer buffer, const BeginRenderPassInfo& info);
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
void CmdPushConstants(CommandBuffer cmd, PipelineLayout layout,
                      ShaderStageFlags flags, uint32_t offset, uint32_t size,
                      const void* values);

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
DescriptorSetLayout CreateDescriptorSetLayout(
    Device device, const DescriptorSetLayoutCreateInfo& info);
void DestroyDescriptorSetLayout(DescriptorSetLayout layout);

// Descriptor Set Pool.
struct DescriptorPoolSize {
  DescriptorType type;
  uint32_t count;
  DescriptorPoolSize() = default;
  DescriptorPoolSize(DescriptorType type, uint32_t count)
      : type(type), count(count) {}
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
struct DescriptorImageInfo {
  ImageView image_view;
  Sampler sampler;

  DescriptorImageInfo() = default;
  DescriptorImageInfo(ImageView image_view, Sampler sampler)
      : image_view(image_view), sampler(sampler) {}
};
struct DescriptorBufferInfo {
  Buffer buffer;
  uint64_t offset = 0;
  uint64_t range;

  DescriptorBufferInfo() = default;
  DescriptorBufferInfo(Buffer buffer, uint64_t offset, uint64_t range)
      : buffer(buffer), offset(offset), range(range) {}
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
struct CopyDescriptorSet {
  DescriptorSet src_set;
  uint32_t src_binding;
  uint32_t src_array_element = 0;
  DescriptorSet dst_set;
  uint32_t dst_binding;
  uint32_t dst_array_element = 0;
  uint32_t descriptor_count = 0;
};
void UpdateDescriptorSets(Device device, uint32_t descriptor_write_count,
                          WriteDescriptorSet* descriptor_writes,
                          uint32_t descriptor_copy_count = 0,
                          CopyDescriptorSet* descriptor_copies = nullptr);

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

// Shader Modules.
ShaderModule CreateShaderModule(Device device, const uint32_t* code,
                                size_t size);
void DestroyShaderModule(Device device, ShaderModule module);

// Helpers.
void StageCopyDataToBuffer(CommandPool pool, Buffer buffer, const void* data,
                           uint64_t size);
struct BufferImageCopy {
  uint64_t buffer_offset = 0;
  uint32_t buffer_row_length = 0;
  uint32_t buffer_image_height = 0;
  ImageSubresourceLayers image_subsurface;
  Offset3D image_offset = {0, 0, 0};
  Extent3D image_extent;
  BufferImageCopy() = default;
  BufferImageCopy(uint32_t x, uint32_t y, uint32_t z, uint32_t width,
                  uint32_t height, uint32_t depth)
      : image_offset(x, y, z), image_extent(width, height, depth) {}
};
void StageCopyDataToImage(CommandPool pool, Image image, const void* data,
                          uint64_t size, uint32_t num_regions,
                          const BufferImageCopy* regions);

struct ImageViewCreateInfo {
  Image image;
  ImageViewType type;
  TextureFormat format;
  ImageSubresourceRange subresource_range;
  Swizzle swizzle;

  ImageViewCreateInfo() = default;
  ImageViewCreateInfo(Image image, ImageViewType type, TextureFormat format,
                      ImageSubresourceRange range, Swizzle swizzle = Swizzle())
      : image(image),
        type(type),
        format(format),
        subresource_range(std::move(range)),
        swizzle(std::move(swizzle)) {}
};
ImageView CreateImageView(Device device, const ImageViewCreateInfo& info);
void DestroyImageView(Device device, ImageView view);

Sampler CreateSampler(Device device,
                      SamplerCreateInfo info = SamplerCreateInfo());
void DestroySampler(Device device, Sampler sampler);

// Helpers.
size_t GetVertexAttributeSize(VertexAttributeType attribute);

}  // namespace RenderAPI