#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoords;
layout(location = 2) in vec3 aColor;

layout(location = 0) out vec3 vColor;
layout(location = 1) out vec2 vTexCoords;

layout(binding = 0) uniform UniformBufferObject {
    vec4 offset;
};

void main() {
    gl_Position = vec4(aPosition, 0.0, 1.0) + offset;
    vTexCoords = aTexCoords;
    vColor = aColor;
}
