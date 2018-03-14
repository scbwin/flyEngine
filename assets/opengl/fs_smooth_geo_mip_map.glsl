#version 330

layout (location = 0) out float color;

uniform sampler2D heightMapSampler;

in vec2 uv;

in float weight_out;

void main()
{
	float blurred_color = 0.f;
	float weight_sum = 0.f;
	float offs = 3.f;
	vec2 texel_size = 1.f / vec2(textureSize(heightMapSampler, 0));
	for (float x = -offs; x <= offs; x++) {
		for (float y = -offs; y <= offs; y++) {
			float weight = 1.f - length(normalize(abs(vec2(x, y)) - vec2(offs)));
			blurred_color += texture(heightMapSampler, uv + vec2(x, y) * texel_size).r;
			weight_sum ++;
		}
	}
	blurred_color /= weight_sum;
	color = weight_out * blurred_color + (1.f - weight_out) * texture(heightMapSampler, uv).r;
}