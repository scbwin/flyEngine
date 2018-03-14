#version 330

uniform sampler2D samplerOpacity;
uniform bool opacityEnabled;

in vec2 tex_coord;
in vec3 pos_world_space;

void main()
{
	if (opacityEnabled && texture(samplerOpacity, tex_coord).r < 0.5f) {
		discard;
	}
}