#version 330
 
layout (location = 0) out vec4 color;

uniform sampler2D cloudSampler;
uniform sampler2DArray gBufferSampler;
uniform float time;

in vec2 uv;
in vec4 pos_view_space;

void main()
{
  float distortion = texture(cloudSampler, uv * 2.f).r * 0.02f;
 // color = texture(cloudSampler, uv * 0.5f + time * 0.02f + distortion);
 // color = pow(color, vec4(3.f));
  color.a = pow(texture(cloudSampler, uv * 0.5f + time * 0.02f + distortion).a, 3.f);
  vec2 uv_ndc = uv * 2.f - 1.f;
  color.a *= 1.f - pow(smoothstep(0.f, 1.f, length(uv_ndc)), 0.5f);

  // Smooth particles
  vec2 uv_screen = gl_FragCoord.xy / vec2(textureSize(gBufferSampler, 0));
  float g_buffer_z = texture(gBufferSampler, vec3(uv_screen, 0.f)).z;
  color.a *= smoothstep(0.f, 100.f, distance(g_buffer_z, pos_view_space.z));
  color.a *= smoothstep(0.f, 1000.f, abs(pos_view_space.z));
  
  color.rgb = vec3(1.f);
}