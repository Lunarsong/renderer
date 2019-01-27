cc_library(
  name = "vulkan",
  srcs = [
    "RenderAPI_vulkan.cpp",
    "detail/RenderAPI_vulkan_detail.h",
    "detail/RenderAPI_vulkan_detail.cpp",
  ],
  hdrs = [
    "RenderAPI_vulkan.h"
  ],
  copts = [],
  deps = [
    "@vulkan//:vulkan",
    "@VulkanMemoryAllocator//:VulkanMemoryAllocator",
    "//RenderAPI:RenderAPI",
    "//generational",
    "//generational:generational_vector",
  ],
  visibility = ["//visibility:public"],
)