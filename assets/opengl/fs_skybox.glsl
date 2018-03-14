#version 330
 
layout (location = 0) out vec3 diffuse;
layout (location = 1) out vec3 occlusion;

uniform samplerCube skyboxSampler;

in vec3 tex_coord;

void main()
{
	diffuse = texture(skyboxSampler, tex_coord).rgb;
	occlusion = diffuse;
}