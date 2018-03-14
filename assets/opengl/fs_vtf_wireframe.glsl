#version 330
 
layout (location = 0) out vec3 color;

uniform vec3 col;

in vec2 uv;

void main()
{
	//color = col;
	color = vec3(uv, 0.f);
}