#version 330
 
layout (location = 0) out vec3 pos_cs;
layout (location = 1) out vec3 normal_cs;
layout (location = 2) out vec3 diffuse;
layout (location = 3) out vec3 occlusion;
layout (location = 4) out vec3 specular;
layout (location = 5) out vec3 light;

uniform sampler2D billboardSampler0;
uniform sampler2D billboardSampler1;
uniform float impostorAlpha;

uniform mat4 MVP;

in vec4 pos_view_space;
in vec2 uv;

void main()
{
	vec4 col = mix(texture(billboardSampler0, uv), texture(billboardSampler1, uv), impostorAlpha);
	if (col.a < 0.3f) {
		discard;
	}
	pos_cs = pos_view_space.xyz;
	normal_cs = vec3(0.f, 0.f, 1.f);
	specular = vec3(0.f);
	diffuse = col.rgb;
	occlusion = vec3(0.f);
	light = occlusion;
}