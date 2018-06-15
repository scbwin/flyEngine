#version 330
 
layout (location = 0) out vec3 fragmentColor;

in vec3 tex_coord;

void main()
{
  const vec3 sky_color = vec3(13.f, 24.f, 42.f) / 255.f * 4.f;
  float weight = pow(smoothstep(0.f, 0.5f, abs(tex_coord.y)), 0.75f);
  fragmentColor = mix(vec3(1.f), sky_color, weight);
}