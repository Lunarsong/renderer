#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vColor;
layout(location = 1) in vec2 vTexCoords;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform ColorUniform {
    vec4 color;
};
layout(binding = 2) uniform sampler2D uTexture0;

void main() {
    outColor = texture(uTexture0, vTexCoords) * color;
}
