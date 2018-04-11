#version 330 
layout(location = 0) out vec3 fragmentColor;
in vec3 pos_world;
in vec3 normal_world;
in vec2 uv_out;
in vec3 tangent_world;
in vec3 bitangent_world;
// Uniform variables are the same for each shader variation, the compiler will optimize away unused variables anyway
uniform vec3 d_col;
uniform sampler2D ts_d;
uniform sampler2D ts_a;
uniform sampler2D ts_n;
uniform sampler2D ts_h;
uniform float pm_h;
uniform vec3 lpos_ws; // light position world space
uniform vec3 cp_ws; // camera position world space
uniform vec3 I_in; // light intensity
uniform mat4 w_to_l [4]; // world space to light space
uniform float fs [4]; // frustum_splits
uniform int nfs; // num frustum splits
uniform sampler2DArrayShadow ts_sm;
// Material constants
uniform float ka;
uniform float kd;
uniform float ks;
uniform float s_e;
void main()
{
  vec2 uv = uv_out;
  vec3 l =  normalize(lpos_ws - pos_world);
  vec3 e =  normalize(cp_ws - pos_world);
  float diffuse = clamp(dot(l, normal_world), 0.f, 1.f);
  float specular = pow(clamp(dot(reflect(-l, normal_world), e), 0.f, 1.f), s_e);
  vec3 albedo = texture(ts_d, uv).rgb;
  fragmentColor = I_in * albedo * (ka + kd * diffuse + ks * specular);
  int index = nfs-1;
  for (int i = nfs-2; i >= 0; i--) {
    index -= int(length(e) < fs[i]);
  }
  vec4 shadow_coord = w_to_l[index] * vec4(pos_world, 1.f);
  shadow_coord.xyz /= shadow_coord.w;
  shadow_coord = shadow_coord * 0.5f + 0.5f;
  shadow_coord.z -= 0.000083f;
  fragmentColor *= 1.f - texture(ts_sm, vec4(shadow_coord.xy, index, shadow_coord.z)) * 0.5f;
}
