#include <opengl/GLSLShaderGenerator.h>
#include <Settings.h>
#include <GraphicsSettings.h>

namespace fly
{
  GLSLShaderGenerator::GLSLShaderGenerator()
  {
    _compositeVertexSource.initFromFile(std::string(vertexFileComposite()), GL_VERTEX_SHADER);

    _windParamString = "uniform float " + std::string(time()) + "; \n\
// Global wind params\n\
uniform vec2 " + std::string(windDir()) + "; // Wind direction\n\
uniform vec2 " + std::string(windMovement()) + "; // Wind movement\n\
uniform float " + std::string(windFrequency()) + "; // Wind frequency\n\
uniform float " + std::string(windStrength()) + "; // Wind strength\n\
// Wind params per mesh\n\
uniform float " + std::string(windPivot()) + "; // Wind pivot\n\
uniform float " + std::string(windExponent()) + "; // Weight exponent\n\
uniform vec3 " + std::string(bbMin()) + "; // AABB min xz\n\
uniform vec3 " + std::string(bbMax()) + "; // AABB max xz\n" + std::string(noiseCodeGLSL());

    _windCodeString = "  float weight = pow(smoothstep(0.f, " + std::string(bbMax()) + ".y - " + std::string(bbMin()) + ".y, abs(" + std::string(windPivot()) + " - pos_world.y)), " + std::string(windExponent()) + ");\n\
  pos_world.xz += " + std::string(windDir()) + " * (noise(pos_world.xz * " + std::string(windFrequency()) + " + " + std::string(time()) + " * " + std::string(windMovement()) + ") * 2.f - 1.f) * " + std::string(windStrength()) + " * weight;\n\
  pos_world.xz = clamp(pos_world.xz, " + std::string(bbMin()) + ".xz, " + std::string(bbMax()) + ".xz);\n";
  }
  GLShaderSource GLSLShaderGenerator::createMeshVertexShaderSource(unsigned flags, const GraphicsSettings & settings)
  {
    std::string key = std::string(directory()) + "vs";
    if (flags & MeshRenderFlag::WIND) {
      key += "_wind";
    }
    key += ".glsl";
    for (const auto& s : _vertexSources) {
      if (s._src._key == key && s._flags == flags) { // File already created
        return s._src;
      }
    }
    GLShaderSource src;
    src._key = key;
    src._source = createMeshVertexSource(flags, settings);;
    src._type = GL_VERTEX_SHADER;
    _vertexSources.push_back({ src, flags });
    return src;
  }
  GLShaderSource GLSLShaderGenerator::createMeshFragmentShaderSource(unsigned flags, const GraphicsSettings& settings)
  {
    std::string key = std::string(directory()) + "fs";
    if (flags & MeshRenderFlag::DIFFUSE_MAP) {
      key += "_albedo";
    }
    if (flags & MeshRenderFlag::NORMAL_MAP) {
      key += "_normal";
      if (flags & MeshRenderFlag::PARALLAX_MAP) {
        key += "_parallax";
      }
    }
    if (flags & MeshRenderFlag::ALPHA_MAP) {
      key += "_alpha";
    }
    if (settings.getShadows() || settings.getShadowsPCF()) {
      key += "_shadows";
    }
    if (settings.getShadowsPCF()) {
      key += "_pcf";
    }
    if (settings.gammaEnabled()) {
      key += "_gamma";
    }
    key += ".glsl";
    for (const auto& s : _fragmentSources) {
      if (s._src._key == key && s._flags == flags) { // File already created
        return s._src;
      }
    }
    GLShaderSource src;
    src._key = key;
    src._source = createMeshFragmentSource(flags, settings);
    src._type = GL_FRAGMENT_SHADER;
    _fragmentSources.push_back({ src, flags });
    return src;
  }
  GLShaderSource GLSLShaderGenerator::createMeshVertexShaderDepthSource(unsigned flags, const GraphicsSettings & settings)
  {
    std::string key = std::string(directory()) + "vs_depth";
    if (flags & MeshRenderFlag::WIND) {
      key += "_wind";
    }
    key += ".glsl";
    for (const auto& s : _vertexDepthSources) {
      if (s._src._key == key && s._flags == flags) { // File already created
        return s._src;
      }
    }
    GLShaderSource src;
    src._key = key;
    src._source = createMeshVertexDepthSource(flags, settings);
    src._type = GL_VERTEX_SHADER;
    _vertexDepthSources.push_back({ src, flags });
    return src;
  }
  GLShaderSource GLSLShaderGenerator::createMeshFragmentShaderDepthSource(unsigned flags, const GraphicsSettings & settings)
  {
    std::string key = std::string(directory()) + "fs_depth";
    if (flags & MeshRenderFlag::ALPHA_MAP) {
      key += "_alpha";
    }
    key += ".glsl";
    for (const auto& s : _fragmentDepthSources) {
      if (s._src._key == key && s._flags == flags) { // File already created
        return s._src;
      }
    }
    GLShaderSource src;
    src._key = key;
    src._source = createMeshFragmentDepthSource(flags, settings);
    src._type = GL_FRAGMENT_SHADER;
    _fragmentDepthSources.push_back({ src, flags });
    return src;
  }
  void GLSLShaderGenerator::createCompositeShaderSource(unsigned flags, const GraphicsSettings & gs, GLShaderSource& vertex_src, GLShaderSource& fragment_src)
  {
    vertex_src = _compositeVertexSource;
    auto key = std::string(directory()) + "fs_composite";
    if (flags & CompositeFlag::EXPOSURE) {
      key += "_exposure";
    }
    if (flags & CompositeFlag::GAMMA_INVERSE) {
      key += "_gamma";
    }
    key += ".glsl";
    for (const auto& s : _compositeSources) {
      if (s._src._key == key && s._flags == flags) { // File already created
        fragment_src = s._src;
        return;
      }
    }
    fragment_src._key = key;
    fragment_src._source = createCompositeShaderSource(flags, gs);
    fragment_src._type = GL_FRAGMENT_SHADER;
    _compositeSources.push_back({ fragment_src, flags });
  }
  void GLSLShaderGenerator::clear()
  {
    _fragmentSources.clear();
    _vertexSources.clear();
    _vertexDepthSources.clear();
    _fragmentDepthSources.clear();
    _compositeSources.clear();
  }
  std::string GLSLShaderGenerator::createMeshVertexSource(unsigned flags, const GraphicsSettings & settings) const
  {
    std::string shader_src;
    shader_src += "#version 330\n\
layout(location = 0) in vec3 position;\n\
layout(location = 1) in vec3 normal;\n\
layout(location = 2) in vec2 uv;\n\
layout(location = 3) in vec3 tangent;\n\
layout(location = 4) in vec3 bitangent;\n\
// Shader constant\n\
uniform mat4 VP; \n\
// Model constants\n\
uniform mat4 M;\n\
uniform mat3 M_i;\n\
out vec3 pos_world;\n\
out vec3 normal_world;\n\
out vec2 uv_out;\n\
out vec3 tangent_world;\n\
out vec3 bitangent_world;\n";
    if (settings.depthPrepassEnabled()) {
      shader_src += "invariant gl_Position; \n";
    }
    if (flags & MeshRenderFlag::WIND) {
      shader_src += _windParamString;
    }
    shader_src += "void main()\n\
{\n\
  pos_world = (M * vec4(position, 1.f)).xyz;\n";
    if (flags & MeshRenderFlag::WIND) {
      shader_src += _windCodeString;
    }
    shader_src += "  gl_Position = VP * vec4(pos_world, 1.f);\n\
  normal_world = normalize(M_i * normal);\n\
  uv_out = uv;\n\
  tangent_world = normalize(M_i * tangent);\n\
  bitangent_world = normalize(M_i * bitangent);\n\
}";
    return shader_src;
  }
  std::string GLSLShaderGenerator::createMeshVertexDepthSource(unsigned flags, const GraphicsSettings & settings) const
  {
    std::string shader_src = "#version 330\n\
layout(location = 0) in vec3 position;\n\
layout(location = 2) in vec2 uv;\n";
    if (settings.depthPrepassEnabled()) {
      shader_src += "invariant gl_Position; \n";
    }
    shader_src += "uniform mat4 " + std::string(modelMatrix()) + ";\n\
uniform mat4 " + std::string(viewProjectionMatrix()) + ";\n";
    shader_src += _windParamString;
    shader_src += "out vec2 uv_out;\n\
void main()\n\
{\n";
    shader_src += "  vec4 pos_world = M * vec4(position, 1.f);\n";
    if (flags & MeshRenderFlag::WIND) {
      shader_src += _windCodeString;
    }
    shader_src += "  gl_Position = VP * pos_world;\n";
    shader_src += "  uv_out = uv;\n\
}\n";
    return shader_src;
  }
  std::string GLSLShaderGenerator::createMeshFragmentSource(unsigned flags, const GraphicsSettings& settings) const
  {
    std::string shader_src = "#version 330 \n\
layout(location = 0) out vec3 fragmentColor;\n\
in vec3 pos_world;\n\
in vec3 normal_world;\n\
in vec2 uv_out;\n\
in vec3 tangent_world;\n\
in vec3 bitangent_world;\n\
// Uniform variables are the same for each shader variation, the compiler will optimize away unused variables anyway\n\
uniform vec3 " + std::string(diffuseColor()) + ";\n\
uniform sampler2D " + std::string(diffuseSampler()) + ";\n\
uniform sampler2D " + std::string(alphaSampler()) + ";\n\
uniform sampler2D " + std::string(normalSampler()) + ";\n\
uniform sampler2D " + std::string(heightSampler()) + ";\n\
uniform float " + std::string(parallaxHeightScale()) + "; // Parallax ray scale\n\
uniform float " + std::string(shadowMapBias()) + "; // Shadow map bias\n\
uniform float " + std::string(shadowDarkenFactor()) + "; // Shadow darken factor\n\
uniform float " + std::string(parallaxMinSteps()) + "; // Parallax min steps\n\
uniform float " + std::string(parallaxMaxSteps()) + "; // Parallax max steps\n\
uniform float " + std::string(parallaxBinarySearchSteps()) + "; // Parallax binary search steps\n\
uniform vec3 " + std::string(lightPositionWorld()) + "; // light position world space\n\
uniform vec3 " + std::string(cameraPositionWorld()) + "; // camera position world space\n\
uniform vec3 " + std::string(lightIntensity()) + "; // light intensity\n\
uniform mat4 " + std::string(worldToLightMatrices()) + " [4]; // world space to light space\n\
uniform float " + std::string(frustumSplits()) + " [4]; // frustum_splits\n\
uniform int " + std::string(numfrustumSplits()) + "; // num frustum splits\n";
    shader_src += (settings.getShadowsPCF() ? "uniform sampler2DArrayShadow " : "uniform sampler2DArray ") + std::string(shadowSampler()) + ";\n";
    shader_src += "// Material constants\n\
uniform float " + std::string(ambientConstant()) + ";\n\
uniform float " + std::string(diffuseConstant()) + ";\n\
uniform float " + std::string(specularConstant()) + ";\n\
uniform float " + std::string(specularExponent()) + ";\n\
uniform float " + std::string(gamma()) + ";\n\
void main()\n\
{\n  vec2 uv = uv_out;\n";
    if ((flags & MeshRenderFlag::NORMAL_MAP) || (flags & MeshRenderFlag::PARALLAX_MAP)) {
      shader_src += "  mat3 world_to_tangent = transpose(mat3(tangent_world, bitangent_world, normal_world));\n";
    }
    if ((flags & NORMAL_MAP) && (flags & PARALLAX_MAP)) { // Parallax only in combination with normal mapping
      shader_src += "  vec3 view_dir_ts = world_to_tangent * normalize(" + std::string(cameraPositionWorld()) + " - pos_world);\n";
      if (!settings.getReliefMapping()) {
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
    shader_src += "  vec3 l = " + std::string((((flags & PARALLAX_MAP) || (flags & NORMAL_MAP)) ? "world_to_tangent *" : "")) + " normalize(" + std::string(lightPositionWorld()) + " - pos_world);\n\
  vec3 e = " + std::string((((flags & PARALLAX_MAP) || (flags & NORMAL_MAP)) ? "world_to_tangent *" : "")) + " normalize(" + std::string(cameraPositionWorld()) + " - pos_world);\n";
    if (flags & MeshRenderFlag::NORMAL_MAP) {
      shader_src += "  vec3 normal_ts = normalize((texture(" + std::string(normalSampler()) + ", uv).xyz * 2.f - 1.f));\n\
  float diffuse = clamp(dot(l, normal_ts), 0.f, 1.f);\n\
  float specular = pow(clamp(dot(normalize(e + l), normal_ts), 0.f, 1.f), " + std::string(specularExponent()) + ");\n";
    }
    else {
      shader_src += "  float diffuse = clamp(dot(l, normal_world), 0.f, 1.f);\n\
  float specular = pow(clamp(dot(normalize(e + l), normal_world), 0.f, 1.f), " + std::string(specularExponent()) + ");\n";
    }

    if (flags & MeshRenderFlag::DIFFUSE_MAP) {
      shader_src += "  vec3 albedo = texture(" + std::string(diffuseSampler()) + ", uv).rgb;\n";
    }
    else {
      shader_src += "  vec3 albedo = " + std::string(diffuseColor()) + ";\n";
    }
    if (settings.gammaEnabled()) {
      shader_src += " albedo = pow(albedo, vec3(" + std::string(gamma()) + "));\n";
    }
    shader_src += "  fragmentColor = I_in * albedo * (" + std::string(ambientConstant()) + " + " + std::string(diffuseConstant()) + " * diffuse + " + std::string(specularConstant()) + " * specular);\n";
    if (settings.getShadows() || settings.getShadowsPCF()) {
      shader_src += "  int index = " + std::string(numfrustumSplits()) + "-1;\n\
  for (int i = " + std::string(numfrustumSplits()) + "-2; i >= 0; i--) {\n\
    index -= int(distance(" + std::string(cameraPositionWorld()) + ", pos_world) < " + std::string(frustumSplits()) + "[i]);\n\
  }\n";
      shader_src += "  vec4 shadow_coord = " + std::string(worldToLightMatrices()) + "[index] * vec4(pos_world, 1.f);\n\
  shadow_coord.xyz /= shadow_coord.w;\n\
  shadow_coord = shadow_coord * 0.5f + 0.5f;\n\
  shadow_coord.z -= " + std::string(shadowMapBias()) + ";\n";
      if (!settings.getShadowsPCF()) {
        shader_src += "  if (all(greaterThanEqual(shadow_coord.xyz, vec3(0.f))) && all(lessThanEqual(shadow_coord.xyz, vec3(1.f)))) {\n  ";
      }
      shader_src += std::string("  fragmentColor *= 1.f - ") + (settings.getShadowsPCF() ? "texture(" + std::string(shadowSampler()) + ", vec4(shadow_coord.xy, index, shadow_coord.z))" : "float(shadow_coord.z > texture(" + std::string(shadowSampler()) + ", vec3(shadow_coord.xy, index)).r)") + " * " + std::string(shadowDarkenFactor()) + ";\n";
      if (!settings.getShadowsPCF()) {
        shader_src += "  }\n";
      }
    }
    shader_src += "}\n";
    return shader_src;
  }
  std::string GLSLShaderGenerator::createMeshFragmentDepthSource(unsigned flags, const GraphicsSettings & settings) const
  {
    std::string shader_src = "#version 330\n\
in vec2 uv_out;\n";
    if (flags & ALPHA_MAP) {
      shader_src += "uniform sampler2D " + std::string(alphaSampler()) + ";\n";
    }
    shader_src += "void main()\n\
{\n";
    if (flags & ALPHA_MAP) {
      shader_src += "  if (texture(" + std::string(alphaSampler()) + ", uv_out).r < 0.5f){\n\
    discard;\n\
  }\n";
    }
    shader_src += "}";
    return shader_src;
  }
  std::string GLSLShaderGenerator::createCompositeShaderSource(unsigned flags, const GraphicsSettings & gs) const
  {
    std::string shader_src = "#version 330\n\
layout(location = 0) out vec3 fragmentColor;\n\
uniform sampler2D " + std::string(lightingSampler()) + ";\n\
uniform float " + std::string(exposure()) + ";\n\
uniform float " + std::string(gammaInverse()) + ";\n\
in vec2 uv;\n\
void main()\n\
{\n\
  fragmentColor = textureLod(" + std::string(lightingSampler()) + ", uv, 0.f).rgb;\n";
    if (flags & CompositeFlag::EXPOSURE) {
      shader_src += "  fragmentColor *= " + std::string(exposure()) + ";\n";
    }
    shader_src += "  fragmentColor = fragmentColor / (1.f + fragmentColor);\n";
    if (flags & CompositeFlag::GAMMA_INVERSE) {
      shader_src += "  fragmentColor = pow(fragmentColor, vec3(" + std::string(gammaInverse()) + "));\n";
    }
    return shader_src + "}\n";
  }
}