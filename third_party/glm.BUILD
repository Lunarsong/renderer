cc_library(
    name = "glm",
    includes = ["."],
    srcs = glob(["glm/**/*.hpp"]) + glob(["glm/**/*.h"]),
    hdrs = glob(["glm/**/*.inl"]),
    visibility = ["//visibility:public"],
    defines = [
        "GLM_FORCE_RADIANS",
        "GLM_FORCE_LEFT_HANDED",
    ],
)
