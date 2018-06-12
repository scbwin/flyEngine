#version 330

layout (location = 0) out vec3 fragmentColor;

in vec3 normal_world;
in vec4 pos_world;

uniform vec3 light_dir_inv;
uniform vec3 cam_pos_world;
uniform vec3 terrain_color;
uniform vec3 ground_color;
uniform vec3 ground_color2;
uniform vec2 snow_level;
uniform vec2 ground_level;
uniform float steepness_exponent;
uniform float steepness_exponent_low;
uniform float snow_intensity;
uniform sampler2D ts_terrain;
uniform sampler2D ts_terrain_n;
uniform float uv_scale;
uniform float ambient;
uniform vec4 view_matrix_third_row;
uniform float fog_distance;
uniform vec3 fog_color;
uniform vec3 light_intensity;

void main()
{
	vec3 diffuse = clamp(dot(light_dir_inv, normal_world), 0.f, 1.f) * light_intensity;
	//vec3 specular = max(dot(reflect(-light_dir_inv, normal_world), normalize(cam_pos_world - pos_world.xyz)), 0.f) * light_intensity;
	vec3 ground_col = mix(ground_color2, ground_color, pow(normal_world.y, steepness_exponent_low));
	vec3 albedo = pow(terrain_color, vec3(2.2f));
	albedo = mix(albedo, vec3(snow_intensity), smoothstep(snow_level.x, snow_level.y, pos_world.y) * pow(normal_world.y, steepness_exponent));
	albedo = mix(pow(ground_col, vec3(2.2f)), albedo, smoothstep(ground_level.x, ground_level.y, pos_world.y));
	vec3 col = albedo * (diffuse + ambient);
	float depth_view = dot(view_matrix_third_row, pos_world);
	col = mix(col, fog_color, smoothstep(0.f, fog_distance, depth_view));
	col /= 1.f + col;
	fragmentColor = pow(col, vec3(1.f / 2.2f));
}