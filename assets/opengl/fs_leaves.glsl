#version 330

layout (location = 0) out vec3 pos_cs;
layout (location = 1) out vec3 normal_cs;
layout (location = 2) out vec3 diffuse;
layout (location = 3) out vec3 occlusion;
layout (location = 4) out vec3 specular;
layout (location = 5) out vec3 light;

uniform sampler2D samplerDiffuse;

in vec3 posViewSpace;
in vec2 texCoord;
in mat3 TBN_view;
in vec3 normalViewSpace;
in vec3 normalModelSpace;

void main()
{
	vec4 col = texture(samplerDiffuse, texCoord);
	if (col.a < 0.5f) {
		discard;
	}
	pos_cs = posViewSpace;
	normal_cs = gl_FrontFacing ? normalViewSpace : -normalViewSpace;
	diffuse = col.rgb;
	occlusion = vec3(0.f);
	specular = vec3(0.01f);
	light = occlusion;
}