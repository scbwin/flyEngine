#version 330
 
layout (location = 0) out vec3 fragmentColor;

uniform sampler2D lightingSampler;
uniform sampler2D bloomSampler0;
uniform sampler2D bloomSampler1;
uniform sampler2D bloomSampler2;
uniform sampler2D godRaySampler;
uniform float bloomWeights [3];
uniform sampler2D samplerExposure;
uniform bool gammaCorrectionEnabled;
uniform bool renderGodRays;
uniform bool bloomEnabled;
uniform bool dofEnabled;
uniform bool motionBlurEnabled;
uniform float godRayWeight;
uniform float motionBlurStrength;
uniform sampler2D dofSampler;
uniform sampler2D depthSampler;
uniform mat4 PInverse;
uniform mat4 VPBeforeTimesVPInverse;
uniform vec3 dofParam;
uniform mat4 VP_inverse;
uniform float waterHeight;
uniform float waterDistortionAlpha;
uniform float time;
uniform bool waterDistortion;

in vec2 uv;
	
const vec3 water_color = vec3(57.f, 88.f, 121.f) / 255.f * 0.3f;

float hash(vec2 p) {
	return fract(sin(dot(p, vec2(12.9898f, 78.233f))) * 43758.5453f);
}

vec2 interpolate(vec2 t)
{
	return t * t * t * (t * (t * 6.f - 15.f) + 10.f);
}

float noise(vec2 p)
{
	vec2 start = floor(p);
	vec2 end = start + 1.f;
	vec2 weights = smoothstep(start, end, p);
	return mix(mix(hash(start), hash(vec2(end.x, start.y)), weights.x), mix(hash(vec2(start.x, end.y)), hash(end), weights.x), weights.y);
}

void main()
{	
	vec2 tex_coord = uv;
	if (waterDistortion) {
		float distortion = noise(uv * 32f + vec2(0.f, time * 8.f)) * 0.03f;
		vec2 uv_distorted = uv + vec2(0.f, distortion);
		if (uv_distorted.y > 1.f) {
			uv_distorted.y = 2.f - uv_distorted.y;
		}
		tex_coord = mix(uv_distorted, uv, waterDistortionAlpha);
	}
	fragmentColor = texture(lightingSampler, tex_coord).rgb;
	float depth = texture(depthSampler, tex_coord).r;
	vec4 pos_ndc = vec4(vec3(tex_coord, depth) * 2.f - 1.f, 1.f);

	if (motionBlurEnabled) {
		vec4 previousPos = VPBeforeTimesVPInverse * pos_ndc;	
		previousPos.xy /= previousPos.w;
		previousPos.xy = previousPos.xy * 0.5f + 0.5f;
		int numSamples = 8;
		vec2 blur_vec = tex_coord - previousPos.xy;
		for (int i = 1; i < numSamples; i++) {
			vec2 offset = blur_vec * (float(i) / float(numSamples - 1) - 0.5f) * motionBlurStrength;
			fragmentColor += texture(lightingSampler, tex_coord + offset).rgb;
		}
		fragmentColor /= float(numSamples);
	}
	if (dofEnabled) {
		vec4 pos_view_space = PInverse * pos_ndc;
		float depth_cs = -(pos_view_space.z / pos_view_space.w);
		vec3 blur_color = texture(dofSampler, tex_coord).rgb;
		vec4 p_near_world = VP_inverse * vec4(tex_coord * 2.f - 1.f, -1.f, 1.f);
		p_near_world.y /= p_near_world.w;
		if (p_near_world.y <= waterHeight) {
			blur_color = mix(blur_color, water_color, 0.3f);
			fragmentColor = mix(fragmentColor, blur_color, smoothstep(0.f, 15.f, depth_cs));
		}
		else if (depth_cs >= dofParam.y) {
			fragmentColor = mix(fragmentColor, blur_color, smoothstep(dofParam.y, dofParam.z, depth_cs));
		}
		else {
			fragmentColor = mix(blur_color, fragmentColor, smoothstep(dofParam.x, dofParam.y, depth_cs));
		}
	}
	if (bloomEnabled) {
		fragmentColor += bloomWeights[0] * texture(bloomSampler0, tex_coord).rgb + bloomWeights[1] * texture(bloomSampler1, tex_coord).rgb + bloomWeights[2] * texture(bloomSampler2, tex_coord).rgb;
		const float steps = 20.f;
		float ray = 0.5f - uv.x;
		float delta = ray / steps;
		vec2 coord = uv;
		vec3 lens_flare = vec3(0.f);
		for (float i = 0.f; i < steps; i++, coord.x += delta) {
			lens_flare += texture(bloomSampler2, coord).rgb;
		}
		fragmentColor += lens_flare / steps;
	}
	if (renderGodRays) {
		fragmentColor += vec3(texture(godRaySampler, tex_coord).r) * godRayWeight;
	}
	float exposure = texelFetch(samplerExposure, ivec2(0), 0).r;
	fragmentColor = vec3(1.f) - exp(-fragmentColor * exposure);
	fragmentColor = fragmentColor / (fragmentColor + 1.f);
	if (gammaCorrectionEnabled) {
		fragmentColor = pow(fragmentColor, vec3(1.f / 2.2f)); // gamma correction
	}
}