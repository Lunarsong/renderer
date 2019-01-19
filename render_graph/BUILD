cc_library(
  name = "render_graph",
  srcs = ["render_graph.cpp", "render_graph_cache.cpp", "render_graph_resources.cpp", "render_graph_builder.cpp"],
  hdrs = ["render_graph.h", "render_graph_pass.h", "render_graph_cache.h", "render_graph_resources.h", "render_graph_builder.h"],
  copts = [],
  deps = [
    "//:renderer",
    "//generational",
    "//generational:generational_vector",
  ],
  visibility = ["//visibility:public"],
)
