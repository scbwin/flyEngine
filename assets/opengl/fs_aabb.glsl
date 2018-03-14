#version 330
 
layout (location = 0) out vec3 color;

uniform vec3 col;

void main()
{
	color = col;
}