#version 330
 
layout (location = 0) out vec3 pos_cs;
layout (location = 1) out vec3 normal_cs;
layout (location = 2) out vec3 diffuse;
layout (location = 3) out vec3 occlusion;
layout (location = 4) out vec3 specular;
layout (location = 5) out vec3 light;

in vec3 posViewSpaceOut;
in vec3 normalViewSpace;
in vec3 color;

void main()
{
	pos_cs = posViewSpaceOut;
	occlusion = vec3(0.f);
	specular = vec3(0.001f);
	light = vec3(0.f);
	diffuse = color;
	
	normal_cs = normalViewSpace;
}