#version 330

uniform sampler2D samplerOpacity;
uniform bool opacityEnabled;

in vec2 tex_coord;
in vec3 pos_world_space;

uniform vec3 lightPosWorld;
uniform float near;
uniform float far;

void main()
{
	if (opacityEnabled && texture(samplerOpacity, tex_coord).r < 0.5f) {
		discard;
	}
	
	float dist_to_light = distance(lightPosWorld, pos_world_space);
	gl_FragDepth = (dist_to_light - near) / (far - near);
}