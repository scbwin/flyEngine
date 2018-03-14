#version 330

layout (location = 0) in vec2 vertex;
layout (location = 1) in float weight;

out vec2 uv;
out float weight_out;

uniform vec2 pos_ndc;
uniform float radius;

void main()
{
	gl_Position = vec4(vertex * radius + pos_ndc, 0.f, 1.f);
	uv = gl_Position.xy * 0.5f + 0.5f;
	weight_out = weight;
}