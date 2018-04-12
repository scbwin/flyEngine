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
uniform float pm_h; // Parallax ray scale
uniform float sm_b; // Shadow map bias
uniform float p_min; // Parallax min steps
uniform float p_max; // Parallax max steps
uniform float pbss; // Parallax binary search steps
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
  mat3 world_to_tangent = transpose(mat3(tangent_world, bitangent_world, normal_world));
  vec3 view_dir_ts = world_to_tangent * normalize(cp_ws - pos_world);
  float steps = mix(p_max, p_min, clamp(dot(vec3(0.f, 0.f, 1.f), view_dir_ts), 0.f, 1.f));
  vec2 ray = view_dir_ts.xy * pm_h;
  vec2 delta = ray / steps;
  float layer_delta = 1.f / steps;
  float layer_depth = 1.f - layer_delta;
  uv -= delta;
  for (float i = 0.f; i < steps; i++, uv -= delta, layer_depth -= layer_delta) {
    if(textureLod(ts_h, uv, 0.f).r > layer_depth){
      delta *= 0.5f;
      layer_delta *= 0.5f;
      uv += delta;
      layer_depth += layer_delta;
      for (float i = 0.f, sign; i < pbss; i++, uv += delta * sign, layer_depth += layer_delta * sign){
        sign = (textureLod(ts_h, uv, 0.f).r > layer_depth) ? 1.f : -1.f;
        delta *= 0.5f;
        layer_delta *= 0.5f;
      }
      break;
    }
  }
  vec3 l = world_to_tangent * normalize(lpos_ws - pos_world);
  vec3 e = world_to_tangent * normalize(cp_ws - pos_world);
  vec3 normal_ts = normalize((texture(ts_n, uv).xyz * 2.f - 1.f));
  float diffuse = clamp(dot(l, normal_ts), 0.f, 1.f);
  float specular = pow(clamp(dot(reflect(-l, normal_ts), e), 0.f, 1.f), s_e);
  vec3 albedo = texture(ts_d, uv).rgb;
  fragmentColor = I_in * albedo * (ka + kd * diffuse + ks * specular);
  int index = nfs-1;
  for (int i = nfs-2; i >= 0; i--) {
    index -= int(length(e) < fs[i]);
  }
  vec4 shadow_coord = w_to_l[index] * vec4(pos_world, 1.f);
  shadow_coord.xyz /= shadow_coord.w;
  shadow_coord = shadow_coord * 0.5f + 0.5f;
  shadow_coord.z -= sm_b;
  fragmentColor *= 1.f - texture(ts_sm, vec4(shadow_coord.xy, index, shadow_coord.z)) * 0.5f;
}
