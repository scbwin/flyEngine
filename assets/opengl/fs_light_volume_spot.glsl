#version 330
 
layout (location = 0) out float lightIntensity;

uniform sampler2DShadow shadowMapSampler;
uniform sampler2D samplerSceneDepth;

uniform mat4 V_inverse_VP_light; // view space to light space
uniform mat4 P_inverse;

uniform mat4 V;
uniform float maxDepth;

uniform float numSamples;

uniform vec3 lightPosViewSpace;
uniform vec3 lightDirViewSpace;
uniform float cosThetaU;
uniform float cosThetaP;

uniform float lightFar;

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
		vec4 shadow_coord = V_inverse_VP_light * vec4(pos_view_space, 1.f);
		shadow_coord /= shadow_coord.w;
		shadow_coord = shadow_coord * 0.5f + 0.5f;
		vec3 pos_to_light = lightPosViewSpace - pos_view_space;
		float cos_theta_s = dot(-normalize(pos_to_light), lightDirViewSpace);
		float light_irradiance = 0.f;
		float fully_lit = float(cos_theta_s >= cosThetaP);
		float partially_lit = float(cosThetaU < cos_theta_s && cos_theta_s < cosThetaP) * pow(((cos_theta_s - cosThetaU) / (cosThetaP - cosThetaU)), 2.f);
		light_irradiance += max(fully_lit, partially_lit);
		float constant = 1.f;
		float linear = 1.f;
		float quadratic = 1.f;
		float dist = length(pos_to_light);
		float attenuation = 1.f / (constant + linear * dist + quadratic * dist * dist);
		lightIntensity += texture(shadowMapSampler, shadow_coord.xyz) * light_irradiance * attenuation;
	}
	lightIntensity *= length(ray_dir) * 0.1f; // Avoids light bleeding artifacts
	lightIntensity /= numSamples;
}