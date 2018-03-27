#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

uniform mat4 MVP;

out vec3 pos_out;
out vec3 normal_out;
out vec2 uv_out;
out vec3 tangent_out;
out vec3 bitangent_out;

void main()
{
	gl_Position = MVP * vec4(position, 1.f);
	pos_out = position;
	normal_out = normal;
	uv_out = uv;
	tangent_out = tangent;
	bitangent_out = bitangent;
}