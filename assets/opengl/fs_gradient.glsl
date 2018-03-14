#version 330
 
layout (location = 0) out float gradient_strength;

uniform sampler2D samplerGradX;
uniform sampler2D samplerGradY;

in vec2 uv;

void main()
{
	gradient_strength = length(vec2(texture(samplerGradX, uv).z, texture(samplerGradY, uv).z));
}