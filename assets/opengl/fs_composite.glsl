#version 330
 
layout (location = 0) out vec3 fragmentColor;

uniform sampler2D ts_l;

in vec2 uv;

void main()
{
	fragmentColor = textureLod(ts_l, uv, 0).rgb;
	fragmentColor = fragmentColor / (1.f + fragmentColor);
}