#pragma once

#include <cstdint>
#include <vector>

namespace Renderer {
using HandleType = uint64_t;
static constexpr HandleType kInvalidHandle = ((HandleType)-1);

using Instance = HandleType;
using Device = HandleType;
using SwapChain = HandleType;
using GraphicsPipeline = HandleType;
using RenderPass = HandleType;
using Framebuffer = HandleType;
using Buffer = HandleType;
using CommandPool = HandleType;
using CommandBuffer = HandleType;
using Semaphore = HandleType;
using Fence = HandleType;

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

struct GraphicsPipelineCreateInfo {
  ShaderCreateInfo vertex;
  ShaderCreateInfo fragment;
  VertexInputBindings vertex_input;
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
enum class BufferType { kVertex, kIndex, kUniform };
Buffer CreateBuffer(Device device, BufferType type, uint64_t size);
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
enum class IndexType { kUInt16 = 0, kUInt32 = 1 };
void CmdBindIndexBuffer(CommandBuffer cmd, Buffer buffer, IndexType type,
                        uint64_t offset = 0);
void CmdDraw(CommandBuffer buffer, uint32_t vertex_count,
             uint32_t instance_count = 1, uint32_t first_vertex = 0,
             uint32_t first_instance = 0);
void CmdDrawIndexed(CommandBuffer cmd, uint32_t index_count,
                    uint32_t instance_count = 1, uint32_t first_index = 0,
                    int32_t vertex_offset = 0, uint32_t first_instance = 0);

// Semaphores.
Semaphore CreateSemaphore(Device);
void DestroySemaphore(Semaphore);

// Fences.
Fence CreateFence(Device device, bool signaled = false);
void DestroyFence(Fence fence);
void WaitForFences(const Fence* fences, uint32_t count, bool wait_for_all,
                   uint64_t timeout_ns);
void ResetFences(const Fence* fences, uint32_t count);

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

}  // namespace Renderer