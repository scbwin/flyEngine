#version 330
 
layout (location = 0) out vec3 fragmentColor;
layout (location = 1) out vec3 viewSpaceNormal;

in float pos_y;

void main()
{
	const vec3 sky_color = vec3(13.f, 24.f, 42.f) / 255.f * 4.f;
	float weight = pow(smoothstep(0.f, 0.5f, abs(pos_y)), 0.75f);
	fragmentColor = mix(vec3(1.f), sky_color, weight);
	viewSpaceNormal = vec3(0.f);
}