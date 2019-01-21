# Description
Shanee is messing around with Vulkan and Render Graphs. Go away ヽ(ಠ_ಠ)ノ

# Notes
I currently have a minimal abstraction for Vulkan. The idea was to create something that looks like Vulkan and then implement GL and other things to fit into that. This may have been a terrible idea.

# Requirements
- Windows.
- [Vulkan SDK Version 1.1.92.1](https://vulkan.lunarg.com/sdk/home#sdk/downloadConfirm/1.1.92.1/windows/VulkanSDK-1.1.92.1-Installer.exe), installed at C:/VulkanSDK/1.1.92.1
- [Bazel](https://bazel.build/)

# Example
```
$ bazel run samples/render_graph/cube --experimental_enable_runfiles
```