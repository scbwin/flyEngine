#version 330

layout (location = 0) out vec3 color;

in vec2 uv;

uniform sampler2D sceneSampler;
uniform sampler2D lensFlareSampler;

void main()
{
	color = texture(lensFlareSampler, uv).rgb + texture(sceneSampler, uv).rgb;
}