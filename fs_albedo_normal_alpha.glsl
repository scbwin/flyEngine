#version 330 
layout(location = 0) out vec3 fragmentColor;
in vec3 pos_view;
in vec3 normal_view;
in vec2 uv_out;
in vec3 tangent_view;
in vec3 bitangent_view;
// Uniform variables are the same for each shader variation, the compiler will optimize away unused variables anyway
uniform vec3 d_col;
uniform sampler2D ts_diff;
uniform sampler2D ts_alpha;
uniform sampler2D ts_norm;
uniform vec3 lpos_cs; // light position view space
uniform vec3 I_in; // light intensity
uniform mat4 v_to_l; // world to light space
uniform sampler2D ts_sm;
// Material constants
uniform float ka;
uniform float kd;
uniform float ks;
uniform float s_e;
void main()
{
  vec3 l = normalize(lpos_cs - pos_view); 
  	if (texture(ts_alpha, uv_out).r < 0.5) {
  discard;
  return;
  }
  mat3 TBN = mat3(tangent_view, bitangent_view, normal_view);
  vec3 normal_view_new = normalize(TBN * (texture(ts_norm, uv_out).xyz * 2.f - 1.f));
  float diffuse = clamp(dot(l, normal_view_new), 0.f, 1.f);
  float specular = pow(clamp(dot(reflect(-l, normal_view_new), normalize(-pos_view)), 0.f, 1.f), s_e);
  vec3 albedo = texture(ts_diff, uv_out).rgb;
  fragmentColor = I_in * albedo * (ka + kd * diffuse + ks * specular);
  vec4 shadow_coord = v_to_l * vec4(pos_view, 1.f);
  shadow_coord.xyz /= shadow_coord.w;
  shadow_coord = shadow_coord * 0.5f + 0.5f;
  if (all(greaterThanEqual(shadow_coord.xyz, vec3(0.f))) && all(lessThanEqual(shadow_coord.xyz, vec3(1.f)))) {
    fragmentColor *= 1.f - float(shadow_coord.z - 0.005f > texture(ts_sm, shadow_coord.xy).r) * 0.5f; 
  }
}
