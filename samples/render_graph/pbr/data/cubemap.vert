#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 aPosition;

layout(location = 0) out vec3 vTexCoords;

layout(push_constant) uniform ObjectData {
    mat4 uMatWorldViewProjection;
};

void main() {
    gl_Position = uMatWorldViewProjection * vec4(aPosition, 1.0);
    vTexCoords = aPosition;
    gl_Position = gl_Position.xyww;
}
