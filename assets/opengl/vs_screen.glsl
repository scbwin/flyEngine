#version 330
 
vec2 vertices [4] = vec2[](vec2(-1.f, -1.f), vec2(1.f, -1.f), vec2(-1.f, 1.f), vec2(1.f, 1.f));

out vec2 uv;

void main()
{
	gl_Position = vec4(vertices[gl_VertexID], 0.f, 1.f);
	uv = gl_Position.xy * 0.5f + 0.5f;
}