#include <opengl/GLSLShaderGenerator.h>
#include <fstream>

namespace fly
{
  std::string GLSLShaderGenerator::createMeshFragmentShaderFile(unsigned flags, bool shadows)
  {
    std::string fname = "fs";
    if (flags & MeshRenderFlag::DIFFUSE_MAP) {
      fname += "_albedo";
    }
    if (flags & MeshRenderFlag::NORMAL_MAP) {
      fname += "_normal";
    }
    if (flags & MeshRenderFlag::ALPHA_MAP) {
      fname += "_alpha";
    }
    fname += ".glsl";
    if (std::ifstream(fname).good()) { // File already exists
      return fname;
    }
    return createMeshFragmentFile(fname, flags, shadows);
  }
  std::string GLSLShaderGenerator::createMeshFragmentFile(const std::string & fname, unsigned flags, bool shadows)
  {
    std::string shader_src = "#version 330 \n\
layout(location = 0) out vec3 fragmentColor;\n\
in vec3 pos_view;\n\
in vec3 normal_view;\n\
in vec2 uv_out;\n\
in vec3 tangent_view;\n\
in vec3 bitangent_view;\n\
// Uniform variables are the same for each shader variation, the compiler will optimize away unused variables anyway\n\
uniform vec3 d_col;\n\
uniform sampler2D ts_diff;\n\
uniform sampler2D ts_alpha;\n\
uniform sampler2D ts_norm;\n\
uniform vec3 lpos_cs; // light position view space\n\
uniform vec3 I_in; // light intensity\n\
uniform mat4 v_to_l; // world to light space\n\
uniform sampler2D ts_sm;\n\
// Material constants\n\
uniform float ka;\n\
uniform float kd;\n\
uniform float ks;\n\
uniform float s_e;\n\
void main()\n\
{\n\
  vec3 l = normalize(lpos_cs - pos_view); \n";
    if (flags & MeshRenderFlag::ALPHA_MAP) {
      shader_src += "  	if (texture(ts_alpha, uv_out).r < 0.5) {\n\
  discard;\n\
  return;\n\
  }\n";
    }
    if (flags & MeshRenderFlag::NORMAL_MAP) {
      shader_src += "  mat3 TBN = mat3(tangent_view, bitangent_view, normal_view);\n\
  vec3 normal_view_new = normalize(TBN * (texture(ts_norm, uv_out).xyz * 2.f - 1.f));\n\
  float diffuse = clamp(dot(l, normal_view_new), 0.f, 1.f);\n\
  float specular = pow(clamp(dot(reflect(-l, normal_view_new), normalize(-pos_view)), 0.f, 1.f), s_e);\n";
    }
    else {
      shader_src += "  float diffuse = clamp(dot(l, normal_view), 0.f, 1.f);\n\
  float specular = pow(clamp(dot(reflect(-l, normal_view), normalize(-pos_view)), 0.f, 1.f), s_e);\n";
    }

    if (flags & MeshRenderFlag::DIFFUSE_MAP) {
      shader_src += "  vec3 albedo = texture(ts_diff, uv_out).rgb;\n";
    }
    else {
      shader_src += "  vec3 albedo = d_col;\n";
    }
    shader_src += "  fragmentColor = I_in * albedo * (ka + kd * diffuse + ks * specular);\n";
    if (shadows) {
      shader_src += "  vec4 shadow_coord = v_to_l * vec4(pos_view, 1.f);\n\
  shadow_coord.xyz /= shadow_coord.w;\n\
  shadow_coord = shadow_coord * 0.5f + 0.5f;\n\
  if (all(greaterThanEqual(shadow_coord.xyz, vec3(0.f))) && all(lessThanEqual(shadow_coord.xyz, vec3(1.f)))) {\n\
    fragmentColor *= 1.f - float(shadow_coord.z - 0.005f > texture(ts_sm, shadow_coord.xy).r) * 0.5f; \n\
  }\n";
    }
    shader_src += "}\n";

    std::ofstream os(fname);
    os.write(shader_src.c_str(), shader_src.size());
    os.close();
    return fname;
  }
}