#version 330
 
layout (location = 0) out vec3 fragColor;

uniform sampler2D samplerShaded;
uniform sampler2D samplerBloom0;
uniform sampler2D samplerBloom1;
uniform sampler2D samplerBloom2;

uniform float weight [3];

in vec2 uv;

void main()
{
	vec3 col0 = texture(samplerBloom0, uv).rgb;
	vec3 col1 = texture(samplerBloom1, uv).rgb;
	vec3 col2 = texture(samplerBloom2, uv).rgb;
	fragColor = texture(samplerShaded, uv).rgb 
	+ weight[0] * col0 + weight[1] * col1 + weight[2] * col2;
}