load("//:glslang.bzl", "glslang")

glslang(
  name = "shaders",
  srcs = [
    "data/triangle.vert",
    "data/triangle.frag",
  ],
)

cc_binary(
  name = "gltf",
  srcs = ["main.cpp"],
  copts = [],
  deps = [
    "@glfw//:glfw",
    "@vulkan//:vulkan",
    "@TinyGLTF//:TinyGLTF",
    "@glm//:glm",
    "//:RenderAPI",
    "//samples/common",
    "//render_graph:render_graph",
  ],
  data = [
    ":shaders",
  ],
)
