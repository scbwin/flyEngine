#version 330

layout (location = 0) out float alpha;

uniform sampler2D alphaSampler;

in float fade_out;
in vec2 uv;
uniform float alpha_model;

void main()
{
	alpha = texture(alphaSampler, uv).r * fade_out * alpha_model;
}