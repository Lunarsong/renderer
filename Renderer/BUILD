cc_library(
  name = "Renderer",
  hdrs = [
    "include/Renderer/Material.h",
    "include/Renderer/MaterialInstance.h",
    "include/Renderer/BuilderBase.h",
    "src/detail/Material.h",
    "src/detail/MaterialInstance.h",
  ],
  srcs = [
    "src/Material.cpp",
    "src/MaterialInstance.cpp",
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
