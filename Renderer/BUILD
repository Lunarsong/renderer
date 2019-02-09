cc_library(
  name = "Renderer",
  hdrs = [
    "include/Renderer/Material.h",
    "include/Renderer/MaterialInstance.h",
    "include/Renderer/MaterialParams.h",
    "include/Renderer/BuilderBase.h",
    "src/detail/Material.h",
    "src/detail/MaterialInstance.h",
    "src/detail/MaterialParams.h",
  ],
  srcs = [
    "src/Material.cpp",
    "src/MaterialInstance.cpp",
    "src/MaterialParams.cpp",
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
