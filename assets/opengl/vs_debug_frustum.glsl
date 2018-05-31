#version 330

//uniform vec3 cube_ndc [8];

layout(location = 0) in vec3 cube_ndc;

uniform mat4 t;

void main()
{
	gl_Position = t * vec4(cube_ndc, 1.f);
	//gl_Position = t * vec4(cube_ndc[gl_VertexID], 1.f);
}