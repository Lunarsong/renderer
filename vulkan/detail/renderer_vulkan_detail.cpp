#include "renderer_vulkan_detail.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace Renderer {
bool enable_validation_layers = true;

SwapChainSupportDetails QuerySwapChainSupport(VkSurfaceKHR surface,
                                              VkPhysicalDevice device) {
  SwapChainSupportDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                            &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                         details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
                                            nullptr);
  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &presentModeCount, details.presentModes.data());
  }

  return details;
}

bool CheckValidationLayerSupport(const std::vector<const char*>& layers) {
  uint32_t layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

  std::vector<VkLayerProperties> available_layers(layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  for (const char* layer_name : layers) {
    bool layer_found = false;
    for (const auto& it : available_layers) {
      if (strcmp(layer_name, it.layerName) == 0) {
        layer_found = true;
        break;
      }
    }
    if (!layer_found) {
      std::cout << "Missing validation layer: " << layer_name << std::endl;
      return false;
    }
  }
  return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
              void* pUserData) {
  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pCallback) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pCallback);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT callback,
                                   const VkAllocationCallbacks* pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, callback, pAllocator);
  }
}

void CreateDebugMessenger(InstanceVk& instance) {
  VkDebugUtilsMessengerCreateInfoEXT create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.pfnUserCallback = DebugCallback;
  create_info.pUserData = nullptr;  // Optional

  if (CreateDebugUtilsMessengerEXT(instance.instance, &create_info, nullptr,
                                   &instance.debug_callback) != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug callback!");
  }
}

QueueFamilyIndices FindQueueFamilies(InstanceVk& instance,
                                     VkPhysicalDevice device) {
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());

  QueueFamilyIndices indices;
  int i = 0;
  for (const auto& queueFamily : queueFamilies) {
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, instance.surface,
                                         &presentSupport);
    if (queueFamily.queueCount > 0) {
      if (presentSupport) {
        indices.presentation = i;
      }
      if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        indices.graphics = i;
      }
    }
    i++;
  }
  return indices;
}

bool CheckDeviceExtensionSupport(
    VkPhysicalDevice device,
    const std::vector<const char*>& required_extensions) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       availableExtensions.data());

  for (const auto& extension : required_extensions) {
    if (std::find_if(availableExtensions.cbegin(), availableExtensions.cend(),
                     [&](const VkExtensionProperties& available_extension) {
                       return strcmp(extension,
                                     available_extension.extensionName) == 0;
                     }) == availableExtensions.cend()) {
      return false;
    }
  }
  return true;
}

bool IsDeviceSuitable(InstanceVk& instance, VkPhysicalDevice device,
                      const std::vector<const char*>& required_extensions) {
  bool extensions_supported =
      CheckDeviceExtensionSupport(device, required_extensions);
  if (extensions_supported) {
    SwapChainSupportDetails swapChainSupport =
        QuerySwapChainSupport(instance.surface, device);
    if (swapChainSupport.formats.empty() ||
        swapChainSupport.presentModes.empty()) {
      return false;
    }
  } else {
    return false;
  }

  QueueFamilyIndices indices = FindQueueFamilies(instance, device);
  VkPhysicalDeviceProperties deviceProperties;
  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);
  vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
  std::cout << "Physical device: " << deviceProperties.deviceName << std::endl;
  return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
         deviceFeatures.geometryShader && indices.IsComplete();
  return false;
}

VkPhysicalDevice SelectPhysicalDevice(InstanceVk& instance) {
  const std::vector<const char*> required_device_extensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance.instance, &deviceCount, nullptr);
  if (deviceCount == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance.instance, &deviceCount, devices.data());

  VkPhysicalDevice physical_device;
  for (const auto& device : devices) {
    if (IsDeviceSuitable(instance, device, required_device_extensions)) {
      physical_device = device;
      break;
    }
  }
  if (physical_device == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
  return physical_device;
}

VkDevice CreateLogicalDevice(
    VkPhysicalDevice physical_device, const QueueFamilyIndices& indices,
    const std::vector<const char*>& validation_layers) {
  const std::vector<const char*> required_device_extensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo queueCreateInfo[2] = {};
  queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo[0].queueFamilyIndex = indices.graphics;
  queueCreateInfo[0].queueCount = 1;
  queueCreateInfo[0].pQueuePriorities = &queuePriority;

  queueCreateInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo[1].queueFamilyIndex = indices.presentation;
  queueCreateInfo[1].queueCount = 1;
  queueCreateInfo[1].pQueuePriorities = &queuePriority;

  VkPhysicalDeviceFeatures deviceFeatures = {};

  VkDeviceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = queueCreateInfo;
  createInfo.queueCreateInfoCount =
      (indices.graphics == indices.presentation) ? 1 : 2;
  createInfo.pEnabledFeatures = &deviceFeatures;
  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(required_device_extensions.size());
  createInfo.ppEnabledExtensionNames = required_device_extensions.data();

  if (enable_validation_layers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validation_layers.size());
    createInfo.ppEnabledLayerNames = validation_layers.data();
  } else {
    createInfo.enabledLayerCount = 0;
  }

  VkDevice device;
  if (vkCreateDevice(physical_device, &createInfo, nullptr, &device) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device!");
  }
  return device;
}

VkSurfaceFormatKHR ChooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) {
  if (availableFormats.size() == 1 &&
      availableFormats[0].format == VK_FORMAT_UNDEFINED) {
    return {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
  }
  for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }
  return availableFormats[0];
}

VkPresentModeKHR ChooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> availablePresentModes) {
  VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

  for (const auto& availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentMode;
    } else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      bestMode = availablePresentMode;
    }
  }

  return bestMode;
}

VkExtent2D ChooseSwapExtent(uint32_t width, uint32_t height,
                            const VkSurfaceCapabilitiesKHR& capabilities) {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    VkExtent2D actualExtent = {width, height};

    actualExtent.width = std::max(
        capabilities.minImageExtent.width,
        std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height = std::max(
        capabilities.minImageExtent.height,
        std::min(capabilities.maxImageExtent.height, actualExtent.height));

    return actualExtent;
  }
}

void CreateSwapChainImageViews(DeviceVk& device, SwapChainVk& swap_chain) {
  swap_chain.image_views.resize(swap_chain.images.size());
  for (size_t i = 0; i < swap_chain.image_views.size(); i++) {
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = swap_chain.images[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = swap_chain.format;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device.device, &createInfo, nullptr,
                          &swap_chain.image_views[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create image views!");
    }
  }
}

VkShaderModule CreateShaderModule(VkDevice device, const uint32_t* code,
                                  size_t size) {
  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = size;
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code);

  VkShaderModule module;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &module) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create shader module!");
  }
  return module;
}

size_t GetVertexAttributeSize(VertexAttributeType attribute) {
  switch (attribute) {
    case VertexAttributeType::kFloat:
      return sizeof(float);
    case VertexAttributeType::kVec2:
      return sizeof(float) * 2;
    case VertexAttributeType::kVec3:
      return sizeof(float) * 3;
    case VertexAttributeType::kVec4:
      return sizeof(float) * 4;
    default:
      throw std::runtime_error("Invalid vertex attribute type!");
  }
}

VkFormat GetFormatFromVertexAttributeType(VertexAttributeType attribute) {
  switch (attribute) {
    case VertexAttributeType::kFloat:
      return VK_FORMAT_R32_SFLOAT;
    case VertexAttributeType::kVec2:
      return VK_FORMAT_R32G32_SFLOAT;
    case VertexAttributeType::kVec3:
      return VK_FORMAT_R32G32B32_SFLOAT;
    case VertexAttributeType::kVec4:
      return VK_FORMAT_R32G32B32A32_SFLOAT;
    default:
      throw std::runtime_error("Invalid vertex attribute type!");
  }
}

VkBufferUsageFlagBits BufferUsageToVulkan(BufferType usage) {
  switch (usage) {
    case BufferType::kVertex:
      return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    case BufferType::kIndex:
      return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    case BufferType::kUniform:
      return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    default:
      throw std::runtime_error("Invalid buffer usage!");
  }
  /*typedef enum VkBufferUsageFlagBits {
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT = 0x00000001,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT = 0x00000002,
    VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT = 0x00000004,
    VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT = 0x00000008,
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT = 0x00000010,
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT = 0x00000020,
    VK_BUFFER_USAGE_INDEX_BUFFER_BIT = 0x00000040,
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT = 0x00000080,
    VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT = 0x00000100,
    VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT = 0x00000800,
    VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT = 0x00001000,
    VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT = 0x00000200,
    VK_BUFFER_USAGE_RAY_TRACING_BIT_NV = 0x00000400,
    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_EXT = 0x00020000,
} VkBufferUsageFlagBits;*/
}

}  // namespace Renderer
