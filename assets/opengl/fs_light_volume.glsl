#version 330
 
layout (location = 0) out float lightIntensity;

uniform sampler2DArrayShadow shadowMapSampler;
uniform sampler2D samplerSceneDepth;
uniform mat4 V_inverse_VP_light [4]; // view space to light space
uniform mat4 P_inverse;
uniform vec4 csmDistance;
uniform float maxDepth;
uniform float numSamples;
uniform int level;
uniform float near;
uniform float far;
in vec2 uv; // between 0 and 1

void main()
{
	vec4 p_near_view_space = P_inverse * vec4(uv * 2.f - 1.f, -1.f, 1.f);
	p_near_view_space /= p_near_view_space.w;
	float scene_depth = min(texture(samplerSceneDepth, uv).x, maxDepth);
	vec4 p_far_view_space = P_inverse * vec4(vec3(uv, scene_depth) * 2.f - 1.f, 1.f);
	p_far_view_space /= p_far_view_space.w;
	vec3 ray_dir = p_far_view_space.xyz - p_near_view_space.xyz;
	vec3 delta = ray_dir / numSamples;
	vec3 pos_view_space = p_near_view_space.xyz;
	lightIntensity = 0.f;
	for (float step = 0.f; step < numSamples; step++, pos_view_space += delta) {
		vec4 comparison = vec4(greaterThan(vec4(-pos_view_space.z), csmDistance));
		int cascade_index = int(dot(vec4(1.f), comparison));
		cascade_index = min(cascade_index, 3);
		vec4 shadow_coord = V_inverse_VP_light[cascade_index] * vec4(pos_view_space, 1.f);
		shadow_coord /= shadow_coord.w;
		shadow_coord = shadow_coord * 0.5f + 0.5f;
		lightIntensity += texture(shadowMapSampler, vec4(shadow_coord.xy, cascade_index, shadow_coord.z));
	}
	lightIntensity *= length(ray_dir) / csmDistance[3]; // Avoids light bleeding artifacts
	lightIntensity /= numSamples;
	float average_scene_depth = texelFetch(samplerSceneDepth, ivec2(0, 0), level).r * 2.f - 1.f;
	float average_scene_depth_linear = 2.f * near * far / (far + near - average_scene_depth * (far - near));
	float factor = 1.f - smoothstep(near, csmDistance[3], average_scene_depth_linear);
	lightIntensity *= factor;
}