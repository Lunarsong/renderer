#version 450

layout (location = 0) out vec2 vTexCoords;

void main()
{
	vTexCoords = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(vTexCoords * 2.0f + -1.0f, 0.0f, 1.0f);
}