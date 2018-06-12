#version 450

layout (vertices = 4) out;

out vec4 tc_position [];

uniform vec3 cam_pos_world;
uniform float max_distance;

#define CP(i) gl_in[i].gl_Position.xyz

float computeTessFactor(vec3 pos)
{
	return pow(2.f, mix(6.f, 1.f, min(distance(pos, cam_pos_world), max_distance) / max_distance));
}

void main ()
{
	tc_position[gl_InvocationID] = gl_in[gl_InvocationID].gl_Position;
	vec3 center = vec3(0.f);
	for (uint i = 0; i < 4; i++) {
		center += CP(i) * 0.25f;
	}
	float tess_factor_inner = computeTessFactor(center);
	for (uint i = 0; i < 2; i++) {
		gl_TessLevelInner[i] = tess_factor_inner;
	}
	gl_TessLevelOuter[0] = computeTessFactor((CP(2) + CP(0)) * 0.5f);
	gl_TessLevelOuter[1] = computeTessFactor((CP(2) + CP(3)) * 0.5f);
	gl_TessLevelOuter[2] = computeTessFactor((CP(1) + CP(3)) * 0.5f);
	gl_TessLevelOuter[3] = computeTessFactor((CP(0) + CP(1)) * 0.5f);
}