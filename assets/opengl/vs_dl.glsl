#version 330

const vec2 vertices[4] = vec2[](vec2(-1.f, -1.f),  vec2(1.f, -1.f), vec2(-1.f, 1.f),  vec2(1.f, 1.f));

uniform mat4 MVP;

out vec2 pos;

void main()
{
	pos = vertices[gl_VertexID];
	gl_Position = MVP * vec4(pos, 0.f, 1.f);
	gl_Position.z = gl_Position.w;
}