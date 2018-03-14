#version 330

layout (location = 5) in mat4 transform; // per instance
layout (location = 9) in float scale; // per instance

const vec2 vertices [4] = vec2[](vec2(-0.5f, 0.f), vec2(0.5f, 0.f), vec2(-0.5f, 1.f), vec2(0.5f, 1.f));

uniform mat4 MV;
uniform mat4 P;
uniform vec2 windDir;
uniform float windStrength;
uniform float windFrequency;
uniform float time;

uniform vec3 modelScale;

out vec4 pos_view_space;
out vec2 uv;

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
	mat4 mv_new = MV * transform;
    mv_new[0][0] = modelScale.x * scale;
    mv_new[0][1] = 0.f;
    mv_new[0][2] = 0.f;
    mv_new[1][0] = 0.f;
    mv_new[1][1] = modelScale.y * scale;
    mv_new[1][2] = 0.f;
    mv_new[2][0] = 0.f;
    mv_new[2][1] = 0.f;
   // mv_new[2][2] = modelScale.z * scale;
	
	vec2 terrain_pos = transform[3].xz;
	vec2 time_factor = time * windFrequency * 32.f * windDir;
	float height_factor = pow(vertices[gl_VertexID].y, 2.f);
	float n = noise(terrain_pos * 0.1f - time_factor) + noise(terrain_pos * 0.2f - time_factor);
	vec2 wind = windDir * n * windStrength * height_factor * 0.5f;
	vec4 wind_cs = MV * vec4(wind.x, 0.f, wind.y, 0.f);
	
	pos_view_space = mv_new * vec4(vertices[gl_VertexID], 0.f, 1.f) + wind_cs;
	gl_Position = P * pos_view_space;
	uv = vertices[gl_VertexID] + vec2(0.5f, 0.f);

	int idx = int(hash(terrain_pos) * 4) % 4;
	vec2 idx_2d = vec2(idx % 2, idx / 2);
	uv = uv * 0.5f + idx_2d * 0.5f;
}