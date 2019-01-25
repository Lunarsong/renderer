load("//:glslang.bzl", "glslang")

glslang(
  name = "shaders",
  srcs = [
    "data/cube.vert",
    "data/cube.frag",
  ],
)

cc_binary(
  name = "cube",
  srcs = ["main.cpp"],
  copts = [],
  deps = [
    "@glfw//:glfw",
    "@vulkan//:vulkan",
    "@glm//:glm",
    "//:renderer",
    "//samples/common",
    "//render_graph:render_graph",
  ],
  data = [
    ":shaders",
  ],
)
