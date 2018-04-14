#version 330

layout (location = 0) in vec3 position;
layout (location = 2) in vec2 uv;

// Shader constant
uniform mat4 VP;

// Model constants
uniform mat4 M;

out vec3 pos_world;
out vec2 uv_out;

void main()
{
	pos_world = (M * vec4(position, 1.f)).xyz;
	gl_Position = VP * vec4(pos_world, 1.f);
}