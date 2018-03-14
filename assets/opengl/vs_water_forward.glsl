#version 330
 
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv_in;
layout (location = 3) in vec3 tan;
layout (location = 4) in vec3 bitan;

uniform mat4 MV;
uniform mat4 P;
uniform mat4 MVInverseTranspose;
uniform float uvFrequency;

out vec2 uv_original;
out vec2 uv;
out mat3 TBN_view;
out vec3 pos_view_space;
out vec3 normal_view_space;

void main()
{
	pos_view_space = vec3(MV * vec4(vertex, 1.f));
	gl_Position = P * vec4(pos_view_space, 1.f);
	TBN_view = mat3(normalize(vec3(MV * vec4(tan, 0.f))), normalize(vec3(MV * vec4(bitan, 0.f))), normalize(vec3(MV * vec4(normal, 0.f))));
	uv_original = vec2(uv_in.x, 1.f - uv_in.y);
	uv = uv_original * uvFrequency;
	normal_view_space = normalize((MVInverseTranspose * vec4(normal, 1.f)).xyz);
}