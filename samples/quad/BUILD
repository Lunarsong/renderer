load("//:glslang.bzl", "glslang")

glslang(
  name = "shaders",
  srcs = [
    "data/shader.vert",
    "data/shader.frag",
  ],
)

cc_binary(
  name = "quad",
  srcs = ["main.cpp"],
  copts = [],
  deps = [
    "@glfw//:glfw",
    "@vulkan//:vulkan",
    "//:RenderAPI",
    "//samples/common",
  ],
  data = [
    ":shaders",
  ],
)