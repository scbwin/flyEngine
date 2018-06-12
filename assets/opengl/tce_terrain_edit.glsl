#version 450

layout (quads) in;

in vec4 tc_position [];

out vec4 pos_world;
out vec3 normal_world;

uniform mat4 view_projection_matrix;
uniform vec3 seed;
uniform bool terraces;
uniform float exponent;
uniform float persistence;
uniform float lacunarity;
uniform float amplitude;
uniform float frequency;
uniform uint octaves;

float hash(vec2 p)
{
  return fract(sin(dot(p, seed.xy)) * seed.z);
}
float noise(vec2 p)
{
  vec2 start = floor(p);
  vec2 end = start + 1.f;
  vec2 weights = smoothstep(start, end, p);
  return mix(mix(hash(start), hash(vec2(end.x, start.y)), weights.x), mix(hash(vec2(start.x, end.y)), hash(end), weights.x), weights.y);
}

float createTerrace(float height, vec2 bounds)
{
	float ref = height;
	if (ref >= bounds.x && ref <= bounds.y) {
		height = mix(bounds.x, bounds.y, pow((height - bounds.x) / (bounds.y - bounds.x), 2.f));
	}
	return height;
}

float getHeight(vec2 pos)
{
	float height = 0.f;
	float amp = amplitude;
	float frequ = frequency;
	for (uint i = 0; i < octaves; i++) {
		height += amp * noise(pos * frequ);
		amp *= persistence;
		frequ *= lacunarity;
	}
	height = pow(height, exponent);
	if (terraces) {
		height = createTerrace(height, vec2(0.f, 3.f));
		height = createTerrace(height, vec2(5.f, 8.f));
		height = createTerrace(height, vec2(10.f, 13.f));
		height = createTerrace(height, vec2(16.f, 19.f));
	}
	return height;
}

void main()
{
	pos_world = mix(mix(tc_position[0], tc_position[1], gl_TessCoord.x), mix(tc_position[2], tc_position[3], gl_TessCoord.x), 1.f - gl_TessCoord.y);
	pos_world.y = getHeight(pos_world.xz);
	gl_Position = view_projection_matrix * pos_world;
	float ddx = getHeight(pos_world.xz + vec2(0.01f, 0.f)) - getHeight(pos_world.xz - vec2(0.01f, 0.f));
	float ddz = getHeight(pos_world.xz + vec2(0.f, 0.01f)) - getHeight(pos_world.xz - vec2(0.f, 0.01f));
	normal_world = normalize(vec3(ddx, 0.01f, ddz));
}