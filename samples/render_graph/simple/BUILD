cc_binary(
  name = "simple",
  srcs = ["main.cpp"],
  copts = [],
  deps = [
    "@glfw//:glfw",
    "@vulkan//:vulkan",
    "//:renderer",
    "//samples/common",
    "//render_graph:render_graph",
  ],
  data = [
    "data/frag.spv",
    "data/vert.spv",
  ],
)
