#version 330
layout(location = 0) in vec3 position;
layout(location = 2) in vec2 uv;
uniform mat4 MVP;
out vec2 uv_out;
void main()
{
  gl_Position = MVP * vec4(position, 1.f);
  uv_out = uv;
}
