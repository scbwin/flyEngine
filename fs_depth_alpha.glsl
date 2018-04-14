#version 330
in vec2 uv_out;
uniform sampler2D ts_a;
void main()
{
  if (texture(ts_a, uv_out).r < 0.5f){
    discard;
  }
}