load("//:glslang.bzl", "glslang")

glslang(
  name = "shaders",
  srcs = [
    "data/triangle.vert",
    "data/triangle.frag",
  ],
)

cc_binary(
  name = "simple",
  srcs = ["main.cpp"],
  copts = [],
  deps = [
    "@glfw//:glfw",
    "@vulkan//:vulkan",
    "//:RenderAPI",
    "//RenderUtils",
    "//samples/common",
    "//render_graph:render_graph",
  ],
  data = [
    ":shaders",
  ],
)
