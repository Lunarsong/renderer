#version 450

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoords;
layout(location = 2) in vec3 aColor;
layout(location = 3) in vec3 aNormal;

layout(push_constant) uniform LightMVP {
    mat4 uLightMVP;
};


void main()
{
	gl_Position =  uLightMVP * vec4(aPosition, 1.0);
}