#version 330
 
layout (location = 0) out vec3 color;

uniform sampler2DArray gBufferSampler;

uniform vec3 lightPosWorldSpace;
uniform vec3 lightPosViewSpace;
uniform vec3 lightColor;

uniform sampler2D samplerScene;
uniform mat4 VPLight;
uniform mat4 V_inverse;

uniform bool gammaCorrectionEnabled;
uniform samplerCubeShadow shadowSampler;

uniform float near;
uniform float far;
uniform bool shadowsEnabled;

in vec2 uv;

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
	
	float amb = 0.01f;
	vec3 specular_color = pow(lightColor, vec3(2.2f));
	vec3 n = normalize(normal_view_space);
	vec3 l = normalize(lightPosViewSpace - pos_view_space);
	float diff = clamp(dot(n, l), 0, 1);
	vec3 diffuse = diff * diffuse_color * lightColor;
			
	vec3 e = normalize(-pos_view_space);
	vec3 r = reflect(-l, n);
	float spec = clamp(dot(e, r), 0, 1);
	vec3 specular = pow(spec, 64.f) * lightColor * specular_power;

	vec3 ambient = amb * diffuse_color * lightColor;
	
	float constant = 1.f;
	float linear = 1.f;
	float quadratic = 1.f;
	float dist = distance(lightPosViewSpace, pos_view_space);
	float attenuation = 1.f / (constant + linear * dist + quadratic * dist * dist);
	
	float visibility = 1.f;
	if (shadowsEnabled) {
		float weight = 0.925f;
		vec3 pos_world_space = vec3(V_inverse * vec4(pos_view_space, 1.f));
		vec3 light_to_fragment = pos_world_space - lightPosWorldSpace;
		float bias = 0.005f;
		float dist_to_light = (length(light_to_fragment) - near) / (far - near) - bias;
		visibility -= weight * (1.f - texture(shadowSampler, vec4(light_to_fragment, dist_to_light)));
	}
	
	color = texture(samplerScene, uv).rgb;
	color += (ambient + diffuse + specular) * attenuation * visibility; // geometry
}