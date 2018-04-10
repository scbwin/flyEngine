#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

// Shader constant
uniform mat4 P;

// Model constants
uniform mat4 MV;
uniform mat3 MV_i;

out vec3 pos_view;
out vec3 normal_view;
out vec2 uv_out;
out vec3 tangent_view;
out vec3 bitangent_view;

void main()
{
	pos_view = (MV * vec4(position, 1.f)).xyz;
	gl_Position = P * vec4(pos_view, 1.f);
	normal_view = normalize(MV_i * normal);
	uv_out = uv;
	tangent_view = normalize(MV_i * tangent);
	bitangent_view = normalize(MV_i * bitangent);
}