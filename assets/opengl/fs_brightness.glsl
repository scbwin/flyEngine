#version 330
 
layout (location = 0) out vec3 color;

uniform sampler2D lightingSampler;

in vec2 uv;

void main()
{
	vec3 shaded_color = texture(lightingSampler, uv).rgb;
	color = smoothstep(0.9f, 1.f, dot(shaded_color, vec3(0.2126f, 0.7152f, 0.0722f))) * shaded_color;
}