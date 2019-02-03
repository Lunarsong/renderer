cc_library(
  name = "Renderer",
  hdrs = [
    "include/Renderer/Material.h",
    "include/Renderer/BuilderBase.h",
    "src/detail/Material.h",
  ],
  srcs = [
    "src/Material.cpp",
    "src/upcast.h",
    "src/BuilderBase.h",
  ],
  includes = [
    "include"
  ],
  deps = [
    "//:RenderAPI",
    "//RenderUtils",
  ],
  visibility = ["//visibility:public"],
)
