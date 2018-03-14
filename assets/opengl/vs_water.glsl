#version 330

const vec2 vertices[4] = vec2[](vec2(0.f, 0.f),  vec2(0.f, 1.f), vec2(1.f, 0.f), vec2(1.f, 1.f));

uniform mat4 MV;
uniform mat4 P;
uniform mat4 MVInverseTranspose;

out vec2 uv;
out vec3 pos_view_space;

void main()
{
  pos_view_space = (MV * vec4(vertices[gl_VertexID].x, 0.f, vertices[gl_VertexID].y, 1.f)).xyz;
  gl_Position = P * vec4(pos_view_space, 1.f);
  uv = vertices[gl_VertexID];
}