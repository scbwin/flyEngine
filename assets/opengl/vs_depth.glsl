#version 330
 
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

uniform mat4 MVP;

out vec2 tex_coord;

void main()
{
	tex_coord = uv;
	gl_Position = MVP * vec4(vertex, 1.f);
}