#version 330

const vec2 vertices[4] = vec2[](vec2(-0.5f, 0.f), vec2(0.5f, 0.f), vec2(-0.5f, 1.f), vec2(0.5f, 1.f));

layout(location = 0) in vec4 transform;

uniform mat4 MV;
uniform mat4 P;

out vec2 uv;
out vec4 pos_view_space;

void main()
{
  vec3 pos_model_space = transform.xyz;
  float scale = transform.w * 2.f;

  mat4 translation_mat = mat4(1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    pos_model_space.x, pos_model_space.y, pos_model_space.z, 1.f);

  mat4 mv_new = MV * translation_mat;
  mv_new[0][0] = scale * 2.f;
  mv_new[0][1] = 0.f;
  mv_new[0][2] = 0.f;
  mv_new[1][0] = 0.f;
  mv_new[1][1] = scale;
  mv_new[1][2] = 0.f;
  mv_new[2][0] = 0.f;
  mv_new[2][1] = 0.f;
  mv_new[2][2] = scale;

  pos_view_space = mv_new * vec4(vertices[gl_VertexID], 0.f, 1.f);
  gl_Position = P * pos_view_space;
  uv = vertices[gl_VertexID] + vec2(0.5f, 0.f);
}