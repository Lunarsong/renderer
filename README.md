# Description
Shanee is messing around with Vulkan and Render Graphs. Go away ヽ(ಠ_ಠ)ノ

The code is a very basic proof of concept, not yet optimized and not yet clean. Avert your nose from the smell.

# Notes
I currently have a minimal abstraction for Vulkan. The idea was to create something that looks like Vulkan and then implement GL and other things to fit into that. This may have been a terrible idea.

# Requirements
- Windows.
- [Vulkan SDK Version 1.1.92.1](https://vulkan.lunarg.com/sdk/home#sdk/downloadConfirm/1.1.92.1/windows/VulkanSDK-1.1.92.1-Installer.exe), installed at C:/VulkanSDK/1.1.92.1
- [Bazel](https://bazel.build/)

Recommend using *--experimental_enable_runfiles* with bazel, however it requires tweaking:
```
Bazel now supports symlink runfiles tree on Windows with --experimental_enable_runfiles flag. It requires admin right or enabling developer mode on Windows 10 Creator Update (1703) or later.
```

# Example
Run a simple Vulkan cube example:
```
$ bazel run samples/render_graph/cube --experimental_enable_runfiles
```