#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoords;
layout(location = 2) in vec3 aColor;
layout(location = 3) in vec3 aNormal;


layout(location = 0) out vec3 vWorldPosition;
layout(location = 1) out vec3 vColor;
layout(location = 2) out vec2 vTexCoords;
layout(location = 3) out vec3 vNormal;
layout(location = 4) out vec4 vShadowCoord;

struct ObjectMatrices {
    mat4 uMatWorldViewProjection;
    mat4 uMatWorld;
    mat4 uMatNormalsMatrix;
};

layout(std140, set = 0, binding = 0) readonly buffer ObjectData {
    ObjectMatrices uObjectData[];
};

layout(push_constant) uniform DepthMVP {
    mat4 uLightMVP;
};

const mat4 biasMat = mat4(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

void main() {
    gl_Position = uObjectData[gl_InstanceIndex].uMatWorldViewProjection * vec4(aPosition, 1.0);

    vec4 world_position = uObjectData[gl_InstanceIndex].uMatWorld * vec4(aPosition, 1.0);
    vWorldPosition = world_position.xyz / world_position.w;

    vTexCoords = aTexCoords;
    vColor = aColor;
    vNormal = normalize(mat3(uObjectData[gl_InstanceIndex].uMatNormalsMatrix) * aNormal);
    vShadowCoord = (biasMat * uLightMVP) /* * uMatWorld */ * vec4(aPosition, 1.0);
}