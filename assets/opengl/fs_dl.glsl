#version 330
 
layout (location = 0) out vec3 pos_cs;
layout (location = 1) out vec3 normal_cs;
layout (location = 2) out vec3 diffuse;
layout (location = 3) out vec3 occlusion;
layout (location = 4) out vec3 specular;
layout (location = 5) out vec3 light;

in vec2 pos;
uniform vec3 lightColor;

void main()
{
	if (length(pos) > 1.f) {
		discard;
	}
	occlusion = lightColor;
	light = occlusion;
}