#version 330

layout (location = 0) out vec4 color;

uniform sampler2D sampler;

uniform float kernel [9];
uniform int kernelSize;
uniform int kernelSizeHalf;
uniform vec2 texelSize;

in vec2 uv;

void main()
{	
	color = vec4(0.f);
	for (int i = 0; i < kernelSize; i++) {
		color += texture(sampler, uv + texelSize * (i - kernelSizeHalf)) * kernel[i];
	}
}