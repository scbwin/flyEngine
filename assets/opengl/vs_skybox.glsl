#version 330
 
layout (location = 0) in vec3 position;

uniform mat4 VP;

out vec3 tex_coord;

void main()
{
	gl_Position = VP * vec4(position, 1.f);
	gl_Position.z = gl_Position.w; // because the skybox is rendered last
	tex_coord = position;
}