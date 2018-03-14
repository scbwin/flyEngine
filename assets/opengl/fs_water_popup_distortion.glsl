#version 330

layout (location = 0) out vec3 fragColor;

uniform sampler2D sceneSampler;

uniform float time;
uniform float alpha;

in vec2 uv;

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
	float distortion = noise(uv * 32.f + vec2(0.f, time * 8.f)) * 0.03f;
	vec2 uv_distorted = uv + vec2(0.f, distortion);
	if (uv_distorted.y > 1.f) { // mirror in y-direction
		uv_distorted.y = 2.f - uv_distorted.y;
	}
	fragColor = texture(sceneSampler, mix(uv_distorted, uv, alpha)).rgb;

}