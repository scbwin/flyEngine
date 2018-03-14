#version 330
 
layout (location = 0) out vec3 color;

uniform sampler2D samplerShaded;
uniform sampler2D samplerNormal;
uniform sampler2D samplerDuDv;
uniform samplerCube samplerSkybox;
uniform sampler2D samplerPosCs;
uniform sampler2D samplerPosCsBack; // back face position
uniform sampler2D samplerOcclusion;
uniform sampler2D samplerWaterSplash;

uniform int doNormalMapping;
uniform vec3 lightPosViewSpace;
uniform vec3 lightColor;
uniform mat4 MV;
uniform mat4 V_inverse;
uniform float uvFrequency;
uniform float time;
uniform float waveStrength;
uniform float waveFrequency;
uniform float fadeDist;

uniform mat4 PPixelSpace;
uniform float nearPlaneZ;
uniform float maxSteps;
uniform int doSSR;

const int MAX_SPLASHES = 50;

uniform vec2 splashPos [MAX_SPLASHES];
uniform float splashScale [MAX_SPLASHES];
uniform float splashWeight [MAX_SPLASHES];

uniform int numSplashes;

uniform int doGammaCorrection;

in vec2 uv_original;
in vec2 uv;
in vec3 pos_view_space;
in vec3 normal_view_space;
in mat3 TBN_view;

bool traceRay(vec3 cs_origin, vec3 cs_dir, out vec3 hit_color) {
	if (doSSR == 0) {
		return false;
	}
	
	vec2 texture_size = textureSize(samplerPosCs, 0);
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	
	float max_distance = 2000.f;
		
	float ray_length = (cs_origin.z + cs_dir.z * max_distance) > -nearPlaneZ ? 
						(-nearPlaneZ - cs_origin.z) / cs_dir.z : max_distance;
		
	vec3 cs_end_point = cs_origin + cs_dir * ray_length;
	vec4 H0 = PPixelSpace * vec4(cs_origin, 1.f);
	vec4 H1 = PPixelSpace * vec4(cs_end_point, 1.f);
	float k0 = 1.f / H0.w, k1 = 1.f / H1.w;
	
	// Interpolated homogeneous version of the end points
	vec3 Q0 = cs_origin * k0, Q1 = cs_end_point * k1;
	
	// Screen space end points
	vec2 P0 = H0.xy * k0, P1 = H1.xy * k1;
		
	P1 += vec2(dot(P0 - P1, P0 - P1) < 0.0001 ? 0.01 : 0.0);
	
	vec2 delta = P1 - P0;
	
	bool permute = false;
	if (abs(delta.x) < abs(delta.y)) {
		permute = true;
		delta = delta.yx;
		P0 = P0.yx;
		P1 = P1.yx;
	}
	
	float step_dir = sign(delta.x);
	float invdx = step_dir / delta.x;
	
	vec3 dQ = (Q1 - Q0) * invdx;
	float dk = (k1 - k0) * invdx;
	vec2 dP = vec2(step_dir, delta.y * invdx);

	float dist_to_border_y = max(pixel.y, texture_size.y - pixel.y);
	float stride = dist_to_border_y / maxSteps;
		
	dP *= stride; dQ *= stride; dk *= stride;
	ivec2 c = ivec2(gl_FragCoord.xy);
	float jitter = float((c.x + c.y) & 1) * 0.5;
	P0 += dP * jitter; Q0 += dQ * jitter; k0 += dk * jitter;
	
	bool hit = false;
	
	vec2 P = P0;
	vec3 Q = Q0;
	float k = k0;
	vec3 cs_hit_point = Q0 * (1.f / k);
	vec3 cs_delta;
	P += dP;
	Q += dQ;
	k += dk;
	for (float step = 0.f; step < maxSteps && !hit; P += dP, Q += dQ, k += dk, step++) {
		ivec2 pix = permute ? ivec2(round(P.yx)) : ivec2(round(P));
	    vec3 cs_hit_point_new = Q / k;
		hit = texelFetch(samplerOcclusion, pix, 0).rgb == vec3(0.f) 
		&& cs_hit_point_new.z >= texelFetch(samplerPosCsBack, pix, 0).z && cs_hit_point_new.z <= texelFetch(samplerPosCs, pix, 0).z
		//hit = cs_hit_point_new.z <= depth_front && (depth_front - cs_hit_point_new.z) <= 0.5f
		;//&& distance(vec2(pix), vec2(pixel)) >= stride;
		cs_delta = cs_hit_point_new - cs_hit_point;
		cs_hit_point = cs_hit_point_new;
	}
		
	vec3 temp_cs_hit_point = cs_hit_point;
	temp_cs_hit_point -= cs_delta / 2.f;
	float factor = 1.f;
	for (float step = 0.f; step < 5.f && hit; step++) {
		vec4 pix = PPixelSpace * vec4(temp_cs_hit_point, 1.f);
		pix.xy /= pix.w;
		ivec2 px = ivec2(round(pix.xy));
		if (temp_cs_hit_point.z >= texelFetch(samplerPosCsBack, px, 0).z && temp_cs_hit_point.z <= texelFetch(samplerPosCs, px, 0).z) {
			cs_hit_point = temp_cs_hit_point;
			temp_cs_hit_point -= cs_delta / (2.f * factor);
		}
		else {
			temp_cs_hit_point += cs_delta / (2.f * factor);
		}
		factor += 1.f;
	}
		
	vec4 px = PPixelSpace * vec4(cs_hit_point, 1.f);
	px.xyz /= px.w;
	ivec2 hit_pixel = ivec2(round(px.xy));
	
	//float dist_to_border = min(px.x, min(texture_size.x - px.x, min(px.y, texture_size.y - px.y)));
	//float min_dist_to_border = texture_size.y * 0.1f;
	//float alpha = min(dist_to_border / min_dist_to_border, 1.f);
	
	hit_color = texelFetch(samplerShaded, hit_pixel, 0).rgb;
	
	return hit;
}

void main()
{
	vec2 screen_uv = gl_FragCoord.xy / textureSize(samplerShaded, 0);

	float refraction_weight = 0.5f;
	
	vec3 diffuse_color = vec3(124.f, 158.f, 211.f) / 255.f;
	if (doGammaCorrection == 1) {
		diffuse_color = pow(diffuse_color, vec3(2.2f));
	}
	
	vec2 distortion1 = (texture(samplerDuDv, uv - time * waveFrequency).xy * 2.f - 1.f);
	vec2 distortion2 = (texture(samplerDuDv, vec2(uv.x, 1.f - uv.y) - time * waveFrequency).xy * 2.f - 1.f);
	vec2 distortion3 = (texture(samplerDuDv, vec2(1.f - uv.x, uv.y) - time * waveFrequency).xy * 2.f - 1.f);
	vec2 distortion4 = (texture(samplerDuDv, vec2(1.f - uv.x, 1.f - uv.y) - time * waveFrequency).xy * 2.f - 1.f);
	vec2 distortion = (distortion1 + distortion2 + distortion3 + distortion4) / 4.f;

	vec3 n = normal_view_space;
	if (doNormalMapping == 1) {		
		n = vec3(0.f);
		n += normalize(texture(samplerNormal, uv - time * waveFrequency * 1.14f + distortion * waveStrength).rgb * 2.f - 1.f);
		n += normalize(texture(samplerNormal, vec2(uv.x, 1.f - uv.y) - time * waveFrequency * 1.33f + distortion * waveStrength).rgb * 2.f - 1.f);
		n += normalize(texture(samplerNormal, vec2(1.f - uv.x, 1.f - uv.y) - time * waveFrequency * 1.5f + distortion * waveStrength).rgb * 2.f - 1.f);
		n += normalize(texture(samplerNormal, vec2(1.f - uv.x, uv.y) - time * waveFrequency * 1.83f + distortion * waveStrength).rgb * 2.f - 1.f);
		
		for (int i = 0; i < numSplashes; i++) {
			vec2 splash_pos = splashPos[i];
			splash_pos = (uv_original - splash_pos) / splashScale[i];
			splash_pos = splash_pos * 0.5f + 0.5f;
			n += normalize(texture(samplerWaterSplash, splash_pos + distortion).xyz * 2.f - 1.f) * splashWeight[i];
		}
		
		n = normalize(TBN_view * n);
	}
	
	vec3 cam_to_pos_cs = normalize(pos_view_space - vec3(0.f)); // camera is at origin in camera space
	
	vec3 cs_dir = normalize(reflect(cam_to_pos_cs, n));
	vec3 ws_dir = normalize(vec3(V_inverse * vec4(cs_dir, 0.f)));
	
	vec3 l = normalize(lightPosViewSpace - pos_view_space);
	float diff = clamp(dot(n, l), 0, 1);
	vec3 diffuse = diff * diffuse_color * lightColor;
			
	vec3 e = normalize(-pos_view_space);
	vec3 r = reflect(-l, n);
	float spec = clamp(dot(e, r), 0, 1);
	vec3 specular = pow(spec, 128.f) * lightColor;
	float amb = 0.3f;
	vec3 ambient = amb * diffuse_color;
	
	vec3 lit_color = ambient + diffuse + specular;
	vec3 reflective_color;
	if (gl_FrontFacing && !traceRay(pos_view_space, cs_dir, reflective_color)) {
		reflective_color = texture(samplerSkybox, ws_dir).rgb;
	}
	
	vec3 refractive_color = texture(samplerShaded, screen_uv + distortion / abs(pos_view_space.z)).rgb;
	
	float fresnel = gl_FrontFacing ? clamp(dot(e, n), 0.f, 1.f) : 1.f;
	vec3 reflect_refract_color = mix(reflective_color, refractive_color, fresnel);
	
	//float weight = gl_FrontFacing ? 0.75f * clamp(abs(pos_view_space.z - texture(samplerPosCs, screen_uv).z) / fadeDist, 0.f, 1.f) : 0.25f;
	
	float fade = 1.f - clamp(abs(pos_view_space.z - texture(samplerPosCs, screen_uv).z) / fadeDist, 0.f, 1.f);
	
	float weight = gl_FrontFacing ? 0.75f : 0.25f;
	
	vec3 scene_color = texture(samplerShaded, screen_uv).rgb;
	
	color = mix(lit_color, reflect_refract_color, weight);
	
	color = mix(color, scene_color, fade);
}