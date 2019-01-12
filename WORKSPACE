load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

new_http_archive(
    name = "gtest",
    url = "https://github.com/google/googletest/archive/release-1.7.0.zip",
    sha256 = "b58cb7547a28b2c718d1e38aee18a3659c9e3ff52440297e965f5edffe34b6d0",
    build_file = "gtest.BUILD",
    strip_prefix = "googletest-release-1.7.0",
)

http_archive(
    name = "gbenchmark",
    url = "https://github.com/google/benchmark/archive/v1.4.1.zip",
    strip_prefix = "benchmark-1.4.1",
)

new_git_repository(
    name = "glfw",
    remote = "https://github.com/glfw/glfw.git",
    commit = "c90c7b97109db909577e3bf540b5f884422b7e14",
    build_file = "third_party/glfw.BUILD",
)

VK_SDK_PATH = "C:/VulkanSDK/1.1.92.1"
new_local_repository(
    name = "vulkan",
    path = VK_SDK_PATH,
    build_file = "third_party/vulkan.BUILD",
)

new_git_repository(
    name = "VulkanMemoryAllocator",
    remote = "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git",
    commit = "ae5c4661ecdff608b6d09704092a1cebbc5bc2ef",
    build_file = "third_party/VulkanMemoryAllocator.BUILD",
)