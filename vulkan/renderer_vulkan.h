#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "renderer.h"

namespace Renderer {

struct QueueFamilyIndices {
  int graphics = -1;
  int presentation = -1;

  bool IsComplete() { return (graphics != -1 && presentation != -1); }
};

struct InstanceVk {
  VkInstance instance;
  VkDebugUtilsMessengerEXT debug_callback;
  VkSurfaceKHR surface;
};

struct DeviceVk {
  Instance instance;
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  QueueFamilyIndices indices;
  VkDevice device;
  VkQueue graphics_queue;
  VkQueue present_queue;
};

struct SwapChainVk {
  Device device;
  VkSwapchainKHR swap_chain = VK_NULL_HANDLE;
  std::vector<VkImage> images;
  std::vector<VkImageView> image_views;
  VkFormat format;
  VkExtent2D extent;
};

struct RenderPassVk {
  Device device;
  VkRenderPass pass;
};

struct GraphicsPipelineVk {
  Device device;
  VkPipelineLayout layout;
  VkPipeline pipeline;
};

struct FramebufferVk {
  Device device;
  VkFramebuffer buffer;
};

struct CommandPoolVk {
  Device device;
  VkCommandPool pool;
};

struct CommandBufferVk {
  Device device;
  CommandPool pool;
  VkCommandBuffer buffer;
};

struct SemaphoreVk {
  VkDevice device;
  VkSemaphore semaphore;
};

struct FenceVk {
  VkDevice device;
  VkFence fence;
};

}  // namespace Renderer