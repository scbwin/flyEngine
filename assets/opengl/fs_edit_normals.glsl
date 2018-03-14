#version 330
 
layout (location = 0) out vec3 normal;

uniform sampler2D heightMapSampler;

uniform mat3 kernel_x;
uniform mat3 kernel_y;

in vec2 uv;

void main()
{	
	float grad_x = 0.f;
	vec2 texel_size = 1.f / vec2(textureSize(heightMapSampler, 0));
	
	for (int x = -1; x <= 1; x++) {
		for (int y = -1; y <= 1; y++) {
			grad_x += texture(heightMapSampler, uv + vec2(x, y) * texel_size).r * kernel_x[x + 1][y + 1];
		}
	}
	
	float grad_y = 0.f;
	for (int x = -1; x <= 1; x++) {
		for (int y = -1; y <= 1; y++) {
			grad_y += texture(heightMapSampler, uv + vec2(x, y) * texel_size).r * kernel_y[x + 1][y + 1];
		}
	}
	
	normal = normalize(vec3(grad_x, 2.f, grad_y));
}