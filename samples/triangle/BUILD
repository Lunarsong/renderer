cc_binary(
  name = "triangle",
  srcs = ["main.cpp"],
  copts = [],
  deps = [
    "@glfw//:glfw",
    "@vulkan//:vulkan",
    "//:renderer",
    "//samples/common",
  ],
  data = [
    "data/frag.spv",
    "data/vert.spv",
  ],
)