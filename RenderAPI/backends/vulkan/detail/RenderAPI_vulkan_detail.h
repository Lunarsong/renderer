#pragma once

#include <vulkan/vulkan.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "../RenderAPI_vulkan.h"
#include "vk_mem_alloc.h"

namespace RenderAPI {
extern bool enable_validation_layers;

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

SwapChainSupportDetails QuerySwapChainSupport(VkSurfaceKHR surface,
                                              VkPhysicalDevice device);
bool CheckValidationLayerSupport(const std::vector<const char*>& layers);
VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pCallback);
void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT callback,
                                   const VkAllocationCallbacks* pAllocator);
void CreateDebugMessenger(InstanceVk& instance);
QueueFamilyIndices FindQueueFamilies(InstanceVk& instance,
                                     VkPhysicalDevice device);
bool CheckDeviceExtensionSupport(
    VkPhysicalDevice device,
    const std::vector<const char*>& required_extensions);
bool IsDeviceSuitable(InstanceVk& instance, VkPhysicalDevice device,
                      const std::vector<const char*>& required_extensions);
VkPhysicalDevice SelectPhysicalDevice(InstanceVk& instance);
VkDevice CreateLogicalDevice(VkPhysicalDevice physical_device,
                             const QueueFamilyIndices& indices,
                             const std::vector<const char*>& validation_layers);
VkSurfaceFormatKHR ChooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats);
VkPresentModeKHR ChooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> availablePresentModes);
VkExtent2D ChooseSwapExtent(uint32_t width, uint32_t height,
                            const VkSurfaceCapabilitiesKHR& capabilities);
void CreateSwapChainImageViews(DeviceVk& device, SwapChainVk& swap_chain);

VkShaderModule CreateShaderModule(VkDevice device, const uint32_t* code,
                                  size_t size);
size_t GetVertexAttributeSize(VertexAttributeType attribute);
VkFormat GetFormatFromVertexAttributeType(VertexAttributeType attribute);
VkBufferUsageFlagBits MemoryUsageToVulkan(MemoryUsage usage);
VmaMemoryUsage MemoryUsageToVulkanMemoryAllocator(MemoryUsage usage);

void DebugAsserts();

}  // namespace RenderAPI