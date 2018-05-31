#version 330
 
layout (location = 0) out vec3 fragment_color;
layout (location = 1) out vec3 normal_view;

uniform vec3 c;

void main()
{
	fragment_color = c;
	normal_view = vec3(0.f);
}