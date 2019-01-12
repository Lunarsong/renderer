config_setting(
    name = "darwin",
    values = {"cpu": "darwin"},
    visibility = ["//visibility:public"],
)

config_setting(
    name = "darwin_x86_64",
    values = {"cpu": "darwin_x86_64"},
    visibility = ["//visibility:public"],
)

config_setting(
    name = "x64_windows_msvc",
    values = {"cpu": "x64_windows_msvc"},
    visibility = ["//visibility:public"],
)

config_setting(
    name = "x64_windows",
    values = {"cpu": "x64_windows"},
    visibility = ["//visibility:public"],
)

cc_library(
    name = "glfw",
    deps = select({
       ":x64_windows": [":glfw_windows"],
       ":x64_windows_msvc": [":glfw_windows"],
       "//conditions:default": [":glfw_osx_ubuntu"],
       }),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "glfw_osx_ubuntu",
    srcs = [
        ":glfw_cmake_osx_ubuntu",
    ],
    hdrs = [
        "include/GLFW/glfw3.h",
        "include/GLFW/glfw3native.h",
    ],
    linkopts = [
    ] + select({
      ":darwin": [
        "-framework Cocoa",
        "-framework OpenGL",
        "-framework IOKit",
        "-framework CoreFoundation",
        "-framework CoreVideo"],
       ":darwin_x86_64": [
        "-framework Cocoa",
        "-framework OpenGL",
        "-framework IOKit",
        "-framework CoreFoundation",
        "-framework CoreVideo"],
     "//conditions:default": [
        "-lGL",
        "-lX11",
        "-lXxf86vm",
        "-lpthread",
        "-ldl",
        "-lXrandr",
        "-lXinerama",
        "-lXcursor",
     ],
    }),
    includes = ["include"],
    visibility = ["//visibility:public"],
)

darwin_cmake = "echo 'Building CMAKE' && cd external/glfw && cmake -DBUILD_SHARED_LIBS=OFF -DGLFW_USE_RETINA=ON -DGLFW_USE_CHDIR=ON -DGLFW_USE_MENUBAR=ON -DGLFW_USE_RETINA=ON . && make && cd ../.. && mv external/glfw/src/libglfw3.a $(OUTS)"
ubuntu_cmake = "echo 'Building CMAKE' && cd external/glfw && cmake -DBUILD_SHARED_LIBS=OFF . && make && cd ../.. && mv external/glfw/src/libglfw3.a $(OUTS)"

genrule(
    name = "glfw_cmake_osx_ubuntu",
    srcs = glob(["**"]),
    cmd = select({
    ":darwin": darwin_cmake,
    ":darwin_x86_64": darwin_cmake,
     "//conditions:default": ubuntu_cmake,
     }),
    outs = [
        "libglfw3.a",
    ],
)

windows_cmd = "cd external/glfw; cmake -VAR:GLFW_USE_MSVC_RUNTIME_LIBRARY_DLL=ON -DBUILD_SHARED_LIBS=ON .; C:/Windows/Microsoft.NET/Framework/v4.0.30319/msbuild.exe ALL_BUILD.vcxproj /p:Configuration=Release; cd ../..; mv external/glfw/src/Release/glfw3.dll $(OUTS)"

genrule(
    name = "glfw_cmake_windows",
    cmd = darwin_cmake,
    outs = [
        "glfw3.dll",
    ],
)

cc_library(
    name = "glfw_windows",
    srcs = [
        "src/context.c",
        "src/egl_context.c",
        "src/init.c",
        "src/input.c",
        "src/monitor.c",
        "src/vulkan.c",
        "src/wgl_context.c",
        "src/win32_init.c",
        "src/win32_joystick.c",
        "src/win32_monitor.c",
        "src/win32_thread.c",
        "src/win32_time.c",
        "src/win32_window.c",
        "src/osmesa_context.c",
        "src/window.c",
    ],
    hdrs = [
        "include/GLFW/glfw3.h",
        "include/GLFW/glfw3native.h",
        "src/internal.h",
        "src/win32_platform.h",
        "src/win32_joystick.h",
        "src/wgl_context.h",
        "src/egl_context.h",
        "src/osmesa_context.h",
        "src/mappings.h",
    ],
    defines = [
        "WIN32",
        "_WINDOWS",
        "NDEBUG",
        "_GLFW_WIN32",
        "_CRT_SECURE_NO_WARNINGS",
    ],
    linkopts = [
        "-DEFAULTLIB:opengl32.lib",
        "-DEFAULTLIB:glu32.lib",
        "-DEFAULTLIB:gdi32.lib",
        "-DEFAULTLIB:user32.lib",
        "-DEFAULTLIB:shell32.lib",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
)
