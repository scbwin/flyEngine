#version 330
 
layout (location = 0) out vec3 fragment_color;

uniform vec3 c;

void main()
{
	fragment_color = c;
}