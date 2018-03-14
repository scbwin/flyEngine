#version 330
 
layout (location = 0) out vec4 color;

uniform sampler2D diffuseSampler;

in vec2 tex_coord;

void main()
{
	color = vec4(texture(diffuseSampler, tex_coord).rgb, 1.f);
}