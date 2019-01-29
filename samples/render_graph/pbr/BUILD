load("//:glslang.bzl", "glslang")

glslang(
  name = "shaders",
  srcs = [
    "data/pbr.vert",
    "data/pbr.frag",
    "data/cubemap.vert",
    "data/cubemap.frag",
    "data/tonemap.vert",
    "data/tonemap.frag",
  ],
)

cc_binary(
  name = "pbr",
  srcs = ["main.cpp", "util.h", "util.cpp", "tonemap_pass.h", "tonemap_pass.cpp"],
  copts = [],
  deps = [
    "@glfw//:glfw",
    "@glm//:glm",
    "//:RenderAPI",
    "//RenderUtils",
    "//samples/common",
    "//render_graph:render_graph",
    ":pbr_renderer",
  ],
  data = [
    ":shaders",
    "data/cubemap.ktx",
    "data/irradiance.ktx",
    "data/prefilter.ktx",
    "data/brdf.ktx",
  ],
)

cc_library(
  name = "pbr_renderer",
  srcs = ["renderer.cpp"],
  hdrs = ["renderer.h", "model.h", "scene.h", "camera.h", "view.h"],
  deps = [
    "@glm//:glm",
    "//:RenderAPI",
    "//RenderUtils",
  ],
)