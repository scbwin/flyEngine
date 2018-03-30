#version 330
 
layout (location = 0) out vec3 fragmentColor;

in vec3 pos_view;
in vec3 normal_view;
in vec2 uv_out;
in vec3 tangent_view;
in vec3 bitangent_view;

uniform sampler2D ts_diff;
uniform sampler2D ts_norm;
uniform vec3 lpos_cs; // light position view space

void main()
{	
	vec3 l = normalize(pos_view - lpos_cs);
	mat3 TBN = mat3(tangent_view, bitangent_view, normal_view);
	vec3 normal_view_new = normalize(TBN * (texture(ts_norm, uv_out).xyz * 2.f - 1.f));
	float diffuse = clamp(dot(l, normal_view_new), 0.f, 1.f);
	fragmentColor = texture(ts_diff, uv_out).rgb * diffuse;
}