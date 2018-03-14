#version 330
 
layout (location = 0) out vec3 normal;

uniform sampler2D gradXSampler;
uniform sampler2D gradYSampler;

in vec2 uv;

void main()
{	
	float nx = texture(gradXSampler, uv).r;
	float nz = texture(gradYSampler, uv).r;
	
	normal = normalize(vec3(nx, 2.f, nz));
}