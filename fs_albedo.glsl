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
uniform mat4 v_to_l; // view space to light space
uniform sampler2DShadow ts_sm;
// Material constants
uniform float ka;
uniform float kd;
uniform float ks;
uniform float s_e;
void main()
{
  vec3 l = normalize(lpos_cs - pos_view); 
  float diffuse = clamp(dot(l, normal_view), 0.f, 1.f);
  float specular = pow(clamp(dot(reflect(-l, normal_view), normalize(-pos_view)), 0.f, 1.f), s_e);
  vec3 albedo = texture(ts_diff, uv_out).rgb;
  fragmentColor = I_in * albedo * (ka + kd * diffuse + ks * specular);
  vec4 shadow_coord = v_to_l * vec4(pos_view, 1.f);
  shadow_coord.xyz /= shadow_coord.w;
  shadow_coord = shadow_coord * 0.5f + 0.5f;
  shadow_coord.z -= 0.005f;
  fragmentColor *= 1.f - texture(ts_sm, shadow_coord.xyz) * 0.5f;
}
