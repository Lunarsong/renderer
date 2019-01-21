load("//:glslang.bzl", "glslang")

glslang(
  name = "shaders",
  srcs = [
    "data/triangle.vert",
    "data/triangle.frag",
  ],
)

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
    ":shaders",
  ],
)