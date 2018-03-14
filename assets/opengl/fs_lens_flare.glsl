#version 330

layout (location = 0) out vec4 color;

in vec2 tex_coord;

uniform sampler2D lensFlareSampler;

//uniform vec4 col;

in vec4 col;

void main()
{
	color = texture(lensFlareSampler, tex_coord) * col;
}