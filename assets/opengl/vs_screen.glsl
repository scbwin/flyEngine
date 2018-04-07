#version 330

out vec2 uv;

void main()
{
	uv = vec2(gl_VertexID & 1, gl_VertexID >> 1);
	gl_Position = vec4(uv * 2.f - 1.f, 0.f, 1.f);
}