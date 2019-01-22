"""Execute glslangValidator.
Compiles a shader using glslang.
"""

def _glslang_outputs(srcs):
    dictionary = {}
    for src in srcs:
      name = src.name.split("/")[1]
      output = src.name + ".spv"
      dictionary[name] = output

    return dictionary


def glslang_impl(ctx):
    runfiles = []
    for src in ctx.files.srcs:
      output = getattr(ctx.outputs, src.basename)
      ctx.actions.run_shell(
          inputs = [src],
          outputs = [output],
          command = "C:/VulkanSDK/1.1.92.1/Bin/glslangValidator -V %s -o %s" % (src.path, output.path),
      )
      runfiles.append(output)

    runfiles_ = ctx.runfiles(files = runfiles)
    return [DefaultInfo(runfiles=runfiles_)]

glslang = rule(
    implementation = glslang_impl,
    attrs = {
      "srcs": attr.label_list(mandatory = True, allow_files = True),
    },
    outputs = _glslang_outputs,
)