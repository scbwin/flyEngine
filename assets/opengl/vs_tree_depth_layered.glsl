#version 330
 
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 tan;
layout (location = 4) in vec3 bitan;
layout (location = 5) in mat4 transform; // per instance

uniform mat4 M;
uniform float time;
uniform vec2 windDir;
uniform float windStrength;
uniform float windFrequency;

out vec4 pos_world_space;

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
	vec2 weights = interpolate(p - start);
	float a = hash(start);
	float b = hash(vec2(end.x, start.y));
	float c = hash(vec2(start.x, end.y));
	float d = hash(end);
	return mix(mix(a, b, weights.x), mix(c, d, weights.x), weights.y);
}

void main()
{
	vec2 terrain_pos = transform[3].xz;
	vec2 time_factor = time * windFrequency * 32.f * windDir;
	float height_factor = pow(vertex.y * 0.05f, 2.f);
	float n = noise(terrain_pos * 0.1f - time_factor) + noise(terrain_pos * 0.2f - time_factor);
	vec2 wind = windDir * n * windStrength * height_factor * 0.5f;
	vec4 pos_model_space = transform * vec4(vertex, 1.f);
	pos_model_space.xz += wind;
	pos_world_space = M * pos_model_space;
}