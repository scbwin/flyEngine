#version 330
 
layout (location = 0) out vec3 color;

uniform sampler2DArray gBufferSampler;

uniform vec3 lightPosViewSpace;
uniform vec3 lightColor;

in vec2 uv;

const int NUM_CASCADES = 4;

uniform mat4 [NUM_CASCADES] V_inverse_VPLight; // view space -> world space -> light space
uniform vec4 csmDistance;

uniform mat4 V_inverse;

uniform sampler2DArrayShadow shadowMapSampler;

uniform bool shadowsEnabled;
uniform bool gammaCorrectionEnabled;

uniform int numSamples;

uniform float nearPlane;
uniform float smBias;

const vec2 poisson_disk[16] = vec2 [] (
vec2(-0.586188, -0.234821),
vec2(0.051244, -0.174817),
vec2(-0.165503, -0.765789),
vec2(-0.682485, 0.399709),
vec2(-0.290195, 0.472577),
vec2(-0.51321, 0.812488),
vec2(0.395101, 0.511341),
vec2(0.349977, -0.538359),
vec2(-0.298498, 0.0794576),
vec2(0.649189, -0.0488529),
vec2(0.787949, 0.457096),
vec2(0.0785128, 0.262649),
vec2(0.0958906, 0.809448),
vec2(0.233477, -0.947833),
vec2(-0.66108, -0.735882),
vec2(0.78604, -0.444697)
);

// Returns a random number based on a vec3 and an int.
float random(vec3 seed, int i){
	vec4 seed4 = vec4(seed, i);
	float dot_product = dot(seed4, vec4(12.9898f, 78.233f, 45.164f, 94.673f));
	return fract(sin(dot_product) * 43758.5453f);
}

uniform vec2 smTexelSize;

float computeShadow(int cascade, vec3 pos_view_space)
{
	float shadow = 0.f;
	vec3 shadow_coord = vec3(V_inverse_VPLight[cascade] * vec4(pos_view_space, 1.f));
	shadow_coord = shadow_coord * 0.5f + 0.5f;
	shadow_coord.z -= float(shadow_coord.z > smBias) * smBias;
	float weight = 0.9f / float(numSamples);
	vec3 pos_world_space = vec3(V_inverse * vec4(pos_view_space, 1.f));
	for (int i = 0; i < numSamples; i++) {
		// random number is dependent on the world position in millimeters
		float rand = random(round(fract(pos_world_space) * 384.f), i) * 3.14f;
		mat2 rotation_mat = mat2(cos(rand), sin(rand), -sin(rand), cos(rand));
		vec2 offset = rotation_mat * poisson_disk[i] * smTexelSize;
		shadow_coord.xy += offset;
		shadow += weight * (1.f - texture(shadowMapSampler, vec4(shadow_coord.xy, cascade, shadow_coord.z))) *
			float(all(greaterThan(shadow_coord, vec3(0.f)))) * float(all(lessThan(shadow_coord, vec3(1.f))));
	}
	return shadow;
}

void main()
{	
	vec3 pos_view_space = texture(gBufferSampler, vec3(uv, 0.f)).rgb;
	vec3 normal_view_space = texture(gBufferSampler, vec3(uv, 1.f)).rgb;
	vec3 diffuse_color = texture(gBufferSampler, vec3(uv, 2.f)).rgb;
	vec3 occlusion = texture(gBufferSampler, vec3(uv, 3.f)).rgb;
	float specular_power = texture(gBufferSampler, vec3(uv, 4.f)).r;
	vec3 light = texture(gBufferSampler, vec3(uv, 5.f)).rgb;
	if (gammaCorrectionEnabled) {
		diffuse_color = pow(diffuse_color, vec3(2.2f));
	}
	
	if (light != vec3(0.f)) { // light source
		color = light;
		return;
	}
	
	if (normal_view_space == vec3(0.f)) { // background
		color = diffuse_color;
		return;
	}

	float amb = 0.1f;
	
	vec3 l = normalize(lightPosViewSpace - pos_view_space);
	float diff = clamp(dot(normal_view_space, l), 0.f, 1.f);
	vec3 diffuse = diff * diffuse_color * lightColor;	
	vec3 e = normalize(-pos_view_space);
	vec3 r = reflect(-l, normal_view_space);
	float spec = clamp(dot(e, r), 0.f, 1.f);
	vec3 specular = pow(spec, 64.f) * lightColor * specular_power;

	vec3 ambient = amb * diffuse_color * lightColor;
	float view_space_depth = -pos_view_space.z;
	vec4 comparison = vec4(greaterThan(vec4(view_space_depth), csmDistance));
	int cascade_index = int(dot(vec4(1.f), comparison));
	cascade_index = min(cascade_index, 3);
	
	float visibility = 1.f;
	if (shadowsEnabled) {
		float fade_start = csmDistance[3] * 2.f;
		float fade = pow(1.f - smoothstep(fade_start, fade_start + 10.f, length(pos_view_space)), 0.5f);
		if (cascade_index < 3) {
			float shadow0 = computeShadow(cascade_index, pos_view_space);
			float shadow1 = computeShadow(cascade_index + 1, pos_view_space);
			float comp = csmDistance[cascade_index];
			float factor = smoothstep(comp * 0.8f, comp, view_space_depth);
			visibility -= mix(shadow0, shadow1, factor) * fade;
		}
		else {
			visibility -= computeShadow(3, pos_view_space) * fade;
		}
	}
	color = (ambient + diffuse + specular) * visibility;
}