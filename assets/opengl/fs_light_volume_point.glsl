#version 330
 
layout (location = 0) out float lightIntensity;

uniform samplerCubeShadow shadowSampler;
uniform sampler2D samplerSceneDepth;
uniform mat4 VP_inverse;
uniform float maxDepth;
uniform float numSamples;
uniform vec3 lightPosWorldSpace;
uniform float near;
uniform float far;

in vec2 uv; // between 0 and 1

void main()
{
	vec4 p_near_world = VP_inverse * vec4(uv * 2.f - 1.f, -1.f, 1.f);
	p_near_world /= p_near_world.w;
	float scene_depth = min(texture(samplerSceneDepth, uv).x, maxDepth);
	vec4 p_far_world = VP_inverse * vec4(vec3(uv, scene_depth) * 2.f - 1.f, 1.f);
	p_far_world /= p_far_world.w;
	vec3 ray_dir = p_far_world.xyz - p_near_world.xyz;
	vec3 delta = ray_dir / numSamples;
	vec3 pos_world = p_near_world.xyz;
	lightIntensity = 0.f;
	for (float step = 0.f; step < numSamples; step++, pos_world += delta) {
		vec3 light_to_pos = pos_world - lightPosWorldSpace;
		float dist_to_light = (length(light_to_pos) - near) / (far - near);
		float constant = 1.f;
		float linear = 1.f;
		float quadratic = 1.f;
		float dist = length(light_to_pos);
		float attenuation = 1.f / (constant + linear * dist + quadratic * dist * dist);
		lightIntensity += texture(shadowSampler, vec4(light_to_pos, dist_to_light)) * attenuation;
	}
	lightIntensity *= length(ray_dir) * 0.1f; // Avoids light bleeding artifacts
	lightIntensity /= numSamples;
}