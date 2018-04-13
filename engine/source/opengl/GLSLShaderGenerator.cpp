#include <opengl/GLSLShaderGenerator.h>
#include <fstream>
#include <iostream>
#include <Settings.h>

namespace fly
{
  std::string GLSLShaderGenerator::createMeshFragmentShaderFile(unsigned flags, const Settings& settings)
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
    if (flags & MeshRenderFlag::PARALLAX_MAP) {
      fname += "_parallax";
    }
    fname += ".glsl";
    for (const auto& n : _fnames) {
      if (n == fname) { // File already created
        return fname;
      }
    }
    _fnames.push_back(fname);
    _flags.push_back(flags);
    return createMeshFragmentFile(fname, flags, settings);
  }
  void GLSLShaderGenerator::regenerateShaders(const Settings& settings)
  {
    for (unsigned i = 0; i < _fnames.size(); i++) {
      createMeshFragmentFile(_fnames[i], _flags[i], settings);
    }
  }
  std::string GLSLShaderGenerator::createMeshFragmentFile(const std::string & fname, unsigned flags, const Settings& settings) const
  {
    std::string shader_src = "#version 330 \n\
layout(location = 0) out vec3 fragmentColor;\n\
in vec3 pos_world;\n\
in vec3 normal_world;\n\
in vec2 uv_out;\n\
in vec3 tangent_world;\n\
in vec3 bitangent_world;\n\
// Uniform variables are the same for each shader variation, the compiler will optimize away unused variables anyway\n\
uniform vec3 d_col;\n\
uniform sampler2D " + std::string(diffuseSampler()) + ";\n\
uniform sampler2D " + std::string(alphaSampler()) + ";\n\
uniform sampler2D " + std::string(normalSampler()) + ";\n\
uniform sampler2D " + std::string(heightSampler()) + ";\n\
uniform float " + std::string(parallaxHeightScale()) + "; // Parallax ray scale\n\
uniform float " + std::string(shadowMapBias()) + "; // Shadow map bias\n\
uniform float " + std::string(parallaxMinSteps()) + "; // Parallax min steps\n\
uniform float " + std::string(parallaxMaxSteps()) + "; // Parallax max steps\n\
uniform float " + std::string(parallaxBinarySearchSteps()) + "; // Parallax binary search steps\n\
uniform vec3 lpos_ws; // light position world space\n\
uniform vec3 cp_ws; // camera position world space\n\
uniform vec3 I_in; // light intensity\n\
uniform mat4 w_to_l [4]; // world space to light space\n\
uniform float fs [4]; // frustum_splits\n\
uniform int nfs; // num frustum splits\n";
    shader_src += settings._shadowPercentageCloserFiltering ? "uniform sampler2DArrayShadow ts_sm;\n" : "uniform sampler2DArray ts_sm;\n";
    shader_src += "// Material constants\n\
uniform float ka;\n\
uniform float kd;\n\
uniform float ks;\n\
uniform float s_e;\n\
void main()\n\
{\n  vec2 uv = uv_out;\n";
    if ((flags & MeshRenderFlag::NORMAL_MAP) || (flags & MeshRenderFlag::PARALLAX_MAP)) {
      shader_src += "  mat3 world_to_tangent = transpose(mat3(tangent_world, bitangent_world, normal_world));\n";
    }
    if (flags & PARALLAX_MAP) {
      shader_src += "  vec3 view_dir_ts = world_to_tangent * normalize(cp_ws - pos_world);\n";
      if (!settings._steepParallax) {
       shader_src += "  uv -= view_dir_ts.xy / view_dir_ts.z * (1.f - textureLod(" + std::string(heightSampler()) + ", uv, 0.f).r) * " + std::string(parallaxHeightScale()) + "; \n";
      }
      else {
        //shader_src += "float steps = 32.f;\n
        shader_src += "  float steps = mix(" + std::string(parallaxMaxSteps()) + ", " + std::string(parallaxMinSteps()) + ", clamp(dot(vec3(0.f, 0.f, 1.f), view_dir_ts), 0.f, 1.f));\n\
  vec2 ray = view_dir_ts.xy * " + std::string(parallaxHeightScale()) + ";\n\
  vec2 delta = ray / steps;\n\
  float layer_delta = 1.f / steps;\n\
  float layer_depth = 1.f - layer_delta;\n\
  uv -= delta;\n\
  for (float i = 0.f; i < steps; i++, uv -= delta, layer_depth -= layer_delta) {\n\
    if(textureLod(" + std::string(heightSampler()) + ", uv, 0.f).r > layer_depth){\n\
      delta *= 0.5f;\n\
      layer_delta *= 0.5f;\n\
      uv += delta;\n\
      layer_depth += layer_delta;\n\
      for (float i = 0.f, sign; i < " + std::string(parallaxBinarySearchSteps()) + "; i++, uv += delta * sign, layer_depth += layer_delta * sign){\n\
        sign = (textureLod(" + std::string(heightSampler()) + ", uv, 0.f).r > layer_depth) ? 1.f : -1.f;\n\
        delta *= 0.5f;\n\
        layer_delta *= 0.5f;\n\
      }\n\
      break;\n\
    }\n\
  }\n";
      }
    }
    if (flags & MeshRenderFlag::ALPHA_MAP) {
      shader_src += "  	if (texture(" + std::string(alphaSampler()) + ", uv).r < 0.5) {\n\
    discard;\n\
    return;\n\
  }\n";
    }
    shader_src += "  vec3 l = " + std::string((((flags & PARALLAX_MAP) || (flags & NORMAL_MAP)) ? "world_to_tangent *" : "")) + " normalize(lpos_ws - pos_world);\n\
  vec3 e = " + std::string((((flags & PARALLAX_MAP) || (flags & NORMAL_MAP)) ? "world_to_tangent *" : "")) + " normalize(cp_ws - pos_world);\n";
    if (flags & MeshRenderFlag::NORMAL_MAP) {
  shader_src += "  vec3 normal_ts = normalize((texture(" + std::string(normalSampler()) + ", uv).xyz * 2.f - 1.f));\n\
  float diffuse = clamp(dot(l, normal_ts), 0.f, 1.f);\n\
  float specular = pow(clamp(dot(normalize(e + l), normal_ts), 0.f, 1.f), s_e);\n";
    }
    else {
      shader_src += "  float diffuse = clamp(dot(l, normal_world), 0.f, 1.f);\n\
  float specular = pow(clamp(dot(normalize(e + l), normal_world), 0.f, 1.f), s_e);\n";
    }

    if (flags & MeshRenderFlag::DIFFUSE_MAP) {
      shader_src += "  vec3 albedo = texture(" + std::string(diffuseSampler()) + ", uv).rgb;\n";
    }
    else {
      shader_src += "  vec3 albedo = d_col;\n";
    }
    shader_src += "  fragmentColor = I_in * albedo * (ka + kd * diffuse + ks * specular);\n";
    if (settings._shadows) {
      shader_src += "  int index = nfs-1;\n\
  for (int i = nfs-2; i >= 0; i--) {\n\
    index -= int(distance(cp_ws, pos_world) < fs[i]);\n\
  }\n";
      shader_src += "  vec4 shadow_coord = w_to_l[index] * vec4(pos_world, 1.f);\n\
  shadow_coord.xyz /= shadow_coord.w;\n\
  shadow_coord = shadow_coord * 0.5f + 0.5f;\n\
  shadow_coord.z -= " + std::string(shadowMapBias()) + ";\n";
      if (!settings._shadowPercentageCloserFiltering) {
        shader_src += "  if (all(greaterThanEqual(shadow_coord.xyz, vec3(0.f))) && all(lessThanEqual(shadow_coord.xyz, vec3(1.f)))) {\n  ";
      }
      shader_src += std::string("  fragmentColor *= 1.f - ") + (settings._shadowPercentageCloserFiltering ? "texture(ts_sm, vec4(shadow_coord.xy, index, shadow_coord.z))" : "float(shadow_coord.z > texture(ts_sm, vec3(shadow_coord.xy, index)).r)") + " * 0.5f;\n";
      if (!settings._shadowPercentageCloserFiltering) {
        shader_src += "  }\n";
      }
    }
    shader_src += "}\n";

    std::ofstream os(fname);
    os.write(shader_src.c_str(), shader_src.size());
    return fname;
  }
}