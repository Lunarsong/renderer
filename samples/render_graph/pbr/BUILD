load("//:glslang.bzl", "glslang")

glslang(
  name = "shaders",
  srcs = [
    "data/pbr.vert",
    "data/pbr.frag",
    "data/pbr_no_textures.vert",
    "data/pbr_no_textures.frag",
    "data/cubemap.vert",
    "data/cubemap.frag",
    "data/tonemap.vert",
    "data/tonemap.frag",
    "data/shadow_depth.vert",
    "data/shadow_depth.frag",
  ],
)

cc_binary(
  name = "pbr",
  srcs = ["main.cpp", "util.h", "util.cpp", "tonemap_pass.h", "tonemap_pass.cpp", "MaterialCache.h", "MaterialCache.cpp", "MaterialBits.h"],
  copts = [],
  deps = [
    "@TinyGLTF//:TinyGLTF",
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
    "data/DamagedHelmet.glb",
  ],
)

cc_library(
  name = "pbr_renderer",
  srcs = ["renderer.cpp", "vertex.cpp", "shadow_pass.cpp", "cascade_shadow_pass.cpp", "camera.cpp"],
  hdrs = ["renderer.h", "model.h", "scene.h", "camera.h", "view.h", "vertex.h", "shadow_pass.h", "cascade_shadow_pass.h",],
  deps = [
    "@glm//:glm",
    "//:RenderAPI",
    "//Renderer",
    "//RenderUtils",
    "//render_graph:render_graph",
    "//samples/common",
  ],
)