load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "gtest",
    url = "https://github.com/google/googletest/archive/release-1.7.0.zip",
    sha256 = "b58cb7547a28b2c718d1e38aee18a3659c9e3ff52440297e965f5edffe34b6d0",
    build_file = "@//:gtest.BUILD",
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
    build_file = "@//:third_party/glfw.BUILD",
)

VK_SDK_PATH = "C:/Dev/VulkanSDK/1.1.106.0"
new_local_repository(
    name = "vulkan",
    path = VK_SDK_PATH,
    build_file = "third_party/vulkan.BUILD",
)

new_git_repository(
    name = "VulkanMemoryAllocator",
    remote = "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git",
    commit = "ae5c4661ecdff608b6d09704092a1cebbc5bc2ef",
    build_file = "@//:third_party/VulkanMemoryAllocator.BUILD",
)

new_git_repository(
    name = "glm",
    remote = "https://github.com/g-truc/glm.git",
    commit = "8cd6db11cd8d2bf68aaad86097f15a9f94604dc4",
    init_submodules = 1,
    build_file = "@//:third_party/glm.BUILD",
)

new_git_repository(
    name = "TinyGLTF",
    remote = "https://github.com/syoyo/tinygltf.git",
    commit = "d6b0b0b990b342cf9eab9878b09de5bb7b031ebf",
    build_file = "@//:third_party/TinyGLTF.BUILD",
)

# OpenGL Image.
new_git_repository(
    name = "gli",
    remote = "https://github.com/g-truc/gli.git",
    commit = "dd17acf9cc7fc6e6abe9f9ec69949eeeee1ccd82",
    build_file = "@//:third_party/gli.BUILD",
)