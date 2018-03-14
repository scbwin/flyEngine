#version 330

layout (location = 0) out float color;

uniform sampler2D heightMapSampler;
uniform float height_offset;

in vec2 uv;

in float weight_out;

void main()
{
	float current_height = texture(heightMapSampler, uv).r;
	color = weight_out * (current_height + height_offset) + (1.f - weight_out) * current_height;
	//color = (current_height + height_offset);
}