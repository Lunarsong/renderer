#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vTexCoords;
layout(location = 0) out vec4 outColor;
layout (binding = 0) uniform samplerCube samplerCubeMap;

void main() {
    vec3 color = texture(samplerCubeMap, vTexCoords).rgb;
    outColor = vec4(color, 1.0);
}
