#version 330

layout (location = 0) out vec4 color;

uniform sampler2D sampler;
uniform sampler2D samplerWeight;

uniform float kernel [9];
uniform int kernelSize;
uniform int kernelSizeHalf;
uniform vec2 texelSize;

in vec2 uv;

void main()
{	
	color = texture(sampler, uv);
	float weight_sum = 1.f;
	float closeness = texture(samplerWeight, uv).r;
	for (int i = 0; i < kernelSize; i++) {
		vec2 tex_coord = uv + texelSize * (i - kernelSizeHalf);
		float weight = closeness * kernel[i];
		color += texture(sampler, tex_coord) * weight;
		weight_sum += weight;
	}
	color /= weight_sum;
}