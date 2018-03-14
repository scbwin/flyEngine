#version 330

layout (location = 0) out vec3 pos_cs;
layout (location = 1) out vec3 normal_cs;
layout (location = 2) out vec3 diffuse;
layout (location = 3) out vec3 occlusion;
layout (location = 4) out vec3 specular;
layout (location = 5) out vec3 light;

uniform sampler2D samplerDiffuse;
uniform sampler2D samplerNormal;
uniform sampler2D samplerDepth;
uniform sampler2D samplerOpacity;

uniform vec3 camPosModelSpace;
uniform vec3 diffuseColor;
uniform vec3 lightColor;

in vec3 posViewSpace;
in vec2 texCoord;
in mat3 TBN_view;
in vec3 normalViewSpace;
in vec3 normalModelSpace;

uniform bool diffuseEnabled;
uniform bool normalEnabled;
uniform bool parallaxEnabled;
uniform bool opacityEnabled;

uniform float specularExponent;

uniform mat4 MVInverseTranspose;

vec2 parallaxMapping(vec3 view_dir_ts)
{
	float min_layers = 8.f;
	float max_layers = 24.f;
	float num_layers = mix(max_layers, min_layers, abs(dot(vec3(0.f, 0.f, 1.f), view_dir_ts)));

	float layer_depth = 1.f / num_layers;
	float current_layer_depth = 0.f;
	float heightScale = 0.06f;
	vec2 p = vec2(view_dir_ts.x, view_dir_ts.y) * heightScale;
	vec2 delta_tex_coord = p / num_layers;
	
	vec2 current_tex_coord = texCoord;
	float current_depth_map_value = 1.f - texture(samplerDepth, current_tex_coord).r;
	
	while (current_layer_depth < current_depth_map_value) {
		current_tex_coord -= delta_tex_coord;
		current_depth_map_value = 1.f - texture(samplerDepth, current_tex_coord).r;
		current_layer_depth += layer_depth;
	}
	
	vec2 prev_tex_coord = current_tex_coord + delta_tex_coord;
	
	float after_depth = current_depth_map_value - current_layer_depth;
	float before_depth = 1.f - texture(samplerDepth, prev_tex_coord).r - current_layer_depth + layer_depth;
	
	float weight = after_depth / (after_depth - before_depth);
	
	return prev_tex_coord * weight + current_tex_coord * (1.f - weight);
}

void main()
{
	vec2 tex_coord = texCoord;
	/*if (parallaxEnabled) {
		vec3 cam_pos_ts = transpose(TBN_model) * camPosModelSpace;
		vec3 frag_pos_ts = transpose(TBN_model) * posModelSpace;
		
		vec3 view_dir_ts = normalize(cam_pos_ts - frag_pos_ts);
		tex_coord = parallaxMapping(view_dir_ts);
	}*/
	
	if (opacityEnabled && texture(samplerOpacity, tex_coord).r < 0.5f) {
		discard;
	}
	
	pos_cs = posViewSpace;
	diffuse = diffuseEnabled ? texture(samplerDiffuse, tex_coord).rgb : diffuseColor;
	occlusion = lightColor;
	specular = vec3(0.2f);
	light = lightColor;
	
	if (normalEnabled) {
		normal_cs = texture(samplerNormal, tex_coord).xyz * 2.f - 1.f; // tangent space
		normal_cs = normalize(TBN_view * normal_cs);
		//normal_cs = TBN_model * normal_cs; // model space
		//normal_cs = normalize((MVInverseTranspose * vec4(normal_cs, 1.f)).xyz); // view space
	}
	else {
		normal_cs = normalViewSpace;
	}
}