#version 330
 
layout (location = 0) out vec3 diffuse;

in vec3 tex_coord;

uniform vec3 color;
uniform sampler2D cloudSampler;
uniform vec2 windDir;

uniform float time;

void main()
{
  float factor = abs(tex_coord.y) * float(tex_coord.y >= 0.f);
  vec3 color_gradient = mix(vec3(1.f), color, factor);
  float distortion = texture(cloudSampler, tex_coord.xz * 4.f).r * 0.05f;
  vec2 uv = tex_coord.xz + distortion;
  float frequency = 1.f - pow(abs(tex_coord.y), 0.8f);
  //vec4 cloud_color = pow(texture(cloudSampler, uv * frequency - time * vec2(0.01f, 0.013f) * 2.f * windDir), vec4(3.f)) * factor;
  vec3 cloud_color = vec3(1.f);
  float alpha = pow(texture(cloudSampler, uv * frequency - time * vec2(0.01f, 0.013f) * 0.5f * windDir).a, 3.f) * factor;
  diffuse = mix(color_gradient, cloud_color, alpha);
}