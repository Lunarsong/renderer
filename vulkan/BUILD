cc_library(
  name = "vulkan",
  srcs = [
    "renderer_vulkan.cpp",
    "detail/renderer_vulkan_detail.h",
    "detail/renderer_vulkan_detail.cpp",
  ],
  hdrs = [
    "renderer_vulkan.h"
  ],
  copts = [],
  deps = [
    "@vulkan//:vulkan",
    "//:renderer",
    "//generational",
    "//generational:generational_vector",
  ],
  visibility = ["//visibility:public"],
)