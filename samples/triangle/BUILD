cc_binary(
  name = "triangle",
  srcs = ["main.cpp"],
  copts = [],
  deps = [
    "@glfw//:glfw",
    "@vulkan//:vulkan",
    "//vulkan",
    "//samples/common",
  ],
  data = [
    "data/frag.spv",
    "data/vert.spv",
  ],
)