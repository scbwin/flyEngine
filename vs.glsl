#version 330
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;
// Shader constant
uniform mat4 VP; 
// Model constants
uniform mat4 M;
uniform mat3 M_i;
out vec3 pos_world;
out vec3 normal_world;
out vec2 uv_out;
out vec3 tangent_world;
out vec3 bitangent_world;
void main()
{
  pos_world = (M * vec4(position, 1.f)).xyz;
  gl_Position = VP * vec4(pos_world, 1.f);
  normal_world = normalize(M_i * normal);
  uv_out = uv;
  tangent_world = normalize(M_i * tangent);
  bitangent_world = normalize(M_i * bitangent);
}