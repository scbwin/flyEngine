#version 330
 
layout (location = 0) out vec3 fragmentColor;
layout (location = 1) out vec3 viewSpaceNormal;

in vec3 tex_coord;

uniform vec3 seed;
uniform float exponent;
uniform float persistence;
uniform float lacunarity;
uniform float amplitude;
uniform float frequency;
uniform float time;

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

float getNoise(vec2 pos)
{
	float height = 0.f;
	float amp = amplitude;
	float frequ = frequency;
	for (int i = 0; i < 6; i++) {
		height += amp * noise(pos * frequ);
		amp *= persistence;
		frequ *= lacunarity;
	}
	height = pow(height, exponent);
	return height;
}

void main()
{
	const vec3 sky_color = vec3(13.f, 24.f, 42.f) / 255.f * 24.f;
	float weight = pow(smoothstep(0.f, 0.5f, abs(tex_coord.y)), 0.75f);
	fragmentColor = mix(vec3(amplitude), sky_color, weight);
	//fragmentColor = sky_color;
	vec3 col_new = fragmentColor + vec3(getNoise(tex_coord.xz * length(tex_coord.xz) + time)) * weight;
	fragmentColor = mix(fragmentColor, col_new, weight);
	fragmentColor /= 1.f + fragmentColor;
	viewSpaceNormal = vec3(0.f);
}