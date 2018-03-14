#version 330

layout (location = 0) out vec3 fragColor;

uniform sampler2D sceneSampler;
uniform sampler2D lightSampler;

uniform vec3 lightColor;
uniform float weight;

in vec2 uv; // screen space pixel coordinates between 0 and 1

void main()
{
	fragColor = texture(sceneSampler, uv).rgb + weight * texture(lightSampler, uv).x * lightColor;
}