#version 330
 
layout (location = 0) out vec3 fragmentColor;

in vec3 pos_view;
in vec3 normal_view;
in vec2 uv_out;
in vec3 tangent_view;
in vec3 bitangent_view;

uniform vec3 d_col;
uniform vec3 lpos_cs; // light position view space

void main()
{	
	vec3 l = normalize(pos_view - lpos_cs);
	float diffuse = clamp(dot(l, normal_view), 0.f, 1.f);
	fragmentColor = d_col * diffuse;
}