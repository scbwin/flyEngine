#include <opengl/GLSLShaderGenerator.h>
#include <Settings.h>
#include <GraphicsSettings.h>
#include <Flags.h>

namespace fly
{
  GLSLShaderGenerator::GLSLShaderGenerator() :
    _compositeVertexSource(std::string(vertexFileComposite()), GL_VERTEX_SHADER)
  {
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
  GLShaderSource GLSLShaderGenerator::createMeshVertexShaderSource(unsigned flags, const GraphicsSettings & settings)const
  {
    std::string key = "vs";
    if (flags & MeshRenderFlag::MR_WIND) {
      key += "_wind";
    }
    key += ".glsl";
    GLShaderSource src;
    src._key = key;
    src._source = createMeshVertexSource(flags, settings);;
    src._type = GL_VERTEX_SHADER;
    return src;
  }
  GLShaderSource GLSLShaderGenerator::createMeshFragmentShaderSource(unsigned flags, const GraphicsSettings& settings)const
  {
    std::string key = "fs";
    if (flags & MeshRenderFlag::MR_DIFFUSE_MAP) {
      key += "_albedo";
    }
    if (flags & MeshRenderFlag::MR_NORMAL_MAP) {
      key += "_normal";
      if (flags & MeshRenderFlag::MR_HEIGHT_MAP) {
        key += "_parallax";
      }
    }
    if (flags & MeshRenderFlag::MR_ALPHA_MAP) {
      key += "_alpha";
    }
    if (flags & MeshRenderFlag::MR_REFLECTIVE) {
      key += "_reflective";
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
    GLShaderSource src;
    src._key = key;
    src._source = createMeshFragmentSource(flags, settings);
    src._type = GL_FRAGMENT_SHADER;
    return src;
  }
  GLShaderSource GLSLShaderGenerator::createMeshVertexShaderDepthSource(unsigned flags, const GraphicsSettings & settings)const
  {
    std::string key = "vs_depth";
    if (flags & MeshRenderFlag::MR_WIND) {
      key += "_wind";
    }
    key += ".glsl";
    GLShaderSource src;
    src._key = key;
    src._source = createMeshVertexDepthSource(flags, settings);
    src._type = GL_VERTEX_SHADER;
    return src;
  }
  GLShaderSource GLSLShaderGenerator::createMeshFragmentShaderDepthSource(unsigned flags, const GraphicsSettings & settings)const
  {
    std::string key = "fs_depth";
    if (flags & MeshRenderFlag::MR_ALPHA_MAP) {
      key += "_alpha";
    }
    key += ".glsl";
    GLShaderSource src;
    src._key = key;
    src._source = createMeshFragmentDepthSource(flags, settings);
    src._type = GL_FRAGMENT_SHADER;
    return src;
  }
  void GLSLShaderGenerator::createCompositeShaderSource(const GraphicsSettings & gs, GLShaderSource& vertex_src, GLShaderSource& fragment_src)const
  {
    vertex_src = _compositeVertexSource;
    std::string key = "fs_composite";
    if (gs.getDepthOfField()) {
      key += "_dof";
    }
    if (gs.exposureEnabled()) {
      key += "_exposure";
    }
    if (gs.gammaEnabled()) {
      key += "_gamma";
    }
    key += ".glsl";
    fragment_src._key = key;
    fragment_src._source = createCompositeShaderSource(gs);
    fragment_src._type = GL_FRAGMENT_SHADER;
  }
  void GLSLShaderGenerator::createBlurShaderSource(unsigned flags, const GraphicsSettings & gs, GLShaderSource & vertex_src, GLShaderSource & fragment_src)const
  {
    vertex_src = _compositeVertexSource;
    fragment_src._source = "#version 330 \n\
layout(location = 0) out vec3 fragmentColor;\n\
in vec2 uv;\n\
uniform sampler2D " + std::string(toBlurSampler()) + ";\n\
uniform vec2 " + std::string(texelSize()) + ";\n\
const float blur_weights [" + std::to_string(gs.getBlurWeights().size()) + "] = float[](";
    for (unsigned i = 0; i < gs.getBlurWeights().size(); i++) {
      fragment_src._source += std::to_string(gs.getBlurWeights()[i]);
      fragment_src._source += i == gs.getBlurWeights().size() - 1 ? ");\n" : ",";
    }
    fragment_src._source += "void main()\n\
{\n\
  fragmentColor = vec3(0.f);\n\
  for (int i = -" + std::to_string(gs.getBlurRadius()) + "; i <= " + std::to_string(gs.getBlurRadius()) + "; i++){\n\
    fragmentColor += texture(" + std::string(toBlurSampler()) + ", uv + i * " + std::string(texelSize()) + ").rgb * blur_weights[i + " + std::to_string(gs.getBlurRadius()) + "];\n\
  }\n\
}\n";
    fragment_src._key = "fs_blur";
    fragment_src._type = GL_FRAGMENT_SHADER;
  }
  void GLSLShaderGenerator::createSSRShaderSource(const GraphicsSettings & gs, GLShaderSource & vertex_src, GLShaderSource & fragment_src)const
  {
    vertex_src = _compositeVertexSource;
    fragment_src._source = "#version 330 \n\
layout(location = 0) out vec3 fragmentColor;\n\
in vec2 uv;\n\
uniform sampler2D " + std::string(lightingSampler()) + ";\n\
uniform sampler2D " + std::string(viewSpaceNormalsSampler()) + ";\n\
uniform sampler2D " + std::string(depthSampler()) + ";\n\
uniform mat4 " + std::string(projectionMatrixInverse()) + "; // Inverse projection matrix\n\
uniform mat4 " + std::string(projectionMatrix()) + "; // Projection matrix\n\
uniform vec4 " + std::string(projectionMatrixInverseThirdRow()) + "; // Third row of inverse projection matrix\n\
uniform vec4 " + std::string(projectionMatrixInverseFourthRow()) + "; // Fourth row of inverse projection matrix\n\
void main()\n\
{\n\
  vec3 normal_view = textureLod(" + std::string(viewSpaceNormalsSampler()) + ", uv, 0.f).xyz;\n\
  if (normal_view != vec3(0.f)) {\n\
    vec4 pos_view_h = " + std::string(projectionMatrixInverse()) + " * vec4(vec3(uv, texture(" + std::string(depthSampler()) + ", uv).r) * 2.f - 1.f, 1.f);\n\
    vec3 pos_view = pos_view_h.xyz / pos_view_h.w;\n\
    vec3 ray_dir = reflect(normalize(pos_view), normal_view);\n\
    vec3 ray = ray_dir * max(-pos_view.z * " + std::to_string(gs.getSSRRayLenScale()) + "f, " + std::to_string(gs.getSSRMinRayLen()) + ");\n\
    vec3 delta = ray / " + std::to_string(gs.getSSRSteps()) + "f;\n\
    vec3 ray_pos_view = pos_view + delta;\n\
    for (float i = 0.f; i < " + std::to_string(gs.getSSRSteps()) + "f; i++, ray_pos_view += delta) {\n\
      vec4 ray_pos_h = " + std::string(projectionMatrix()) + " * vec4(ray_pos_view, 1.f);\n\
      vec2 ray_pos_ndc = ray_pos_h.xy / ray_pos_h.w;\n\
      vec2 uv = ray_pos_ndc * 0.5f + 0.5f;\n\
      vec4 comp_ndc = vec4(vec3(uv, texture(" + std::string(depthSampler()) + ", uv).r) * 2.f - 1.f, 1.f);\n\
      float comp_depth = dot(comp_ndc, " + std::string(projectionMatrixInverseThirdRow()) + ") / dot(comp_ndc, " + std::string(projectionMatrixInverseFourthRow()) + ");\n\
      if (ray_pos_view.z < comp_depth) { // Intersection found, the ray traveled behind the depth buffer. \n\
        float sign;\n\
        for (uint i = 0u; i < " + std::to_string(gs.getSSRBinarySteps()) + "u; i++, delta *= 0.5f, ray_pos_view += delta * sign) { // Binary search refinement\n\
          ray_pos_h = " + std::string(projectionMatrix()) + " * vec4(ray_pos_view, 1.f);\n\
          ray_pos_ndc = ray_pos_h.xy / ray_pos_h.w;\n\
          uv = ray_pos_ndc * 0.5f + 0.5f;\n\
          vec4 comp_ndc = vec4(vec3(uv, texture(" + std::string(depthSampler()) + ", uv).r) * 2.f - 1.f, 1.f);\n\
          float comp_depth = dot(comp_ndc, " + std::string(projectionMatrixInverseThirdRow()) + ") / dot(comp_ndc, " + std::string(projectionMatrixInverseFourthRow()) + ");\n\
          sign = ray_pos_view.z < comp_depth ? -1.f : 1.f;\n\
        }\n\
        fragmentColor = textureLod(" + std::string(lightingSampler()) + ", uv, 0.f).rgb;\n\
        return;\n\
      }\n\
    }\n\
  }\n\
  fragmentColor = textureLod(" + std::string(lightingSampler()) + ", uv, 0.f).rgb;\n\
}\n";
    fragment_src._key = "fs_ssr";
    fragment_src._type = GL_FRAGMENT_SHADER;
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
uniform mat4 " + std::string(modelMatrix()) + ";\n\
out vec3 pos_world;\n\
out vec3 normal_local;\n\
out vec2 uv_out;\n\
out vec3 tangent_local;\n\
out vec3 bitangent_local;\n";
    if (settings.depthPrepassEnabled()) {
      shader_src += "invariant gl_Position; \n";
    }
    if (flags & MeshRenderFlag::MR_WIND) {
      shader_src += _windParamString;
    }
    shader_src += "void main()\n\
{\n\
  pos_world = (M * vec4(position, 1.f)).xyz;\n";
    if (flags & MeshRenderFlag::MR_WIND) {
      shader_src += _windCodeString;
    }
    shader_src += "  gl_Position = VP * vec4(pos_world, 1.f);\n\
  normal_local = normal;\n\
  uv_out = uv;\n\
  tangent_local = tangent;\n\
  bitangent_local = bitangent;\n\
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
    if (flags & MeshRenderFlag::MR_WIND) {
      shader_src += _windCodeString;
    }
    shader_src += "  gl_Position = VP * pos_world;\n";
    shader_src += "  uv_out = uv;\n\
}\n";
    return shader_src;
  }
  std::string GLSLShaderGenerator::createMeshFragmentSource(unsigned flags, const GraphicsSettings& settings) const
  {
    bool tangent_space = (flags & MR_NORMAL_MAP) || (flags & MR_HEIGHT_MAP);
    std::string shader_src = "#version 330 \n\
layout(location = 0) out vec3 fragmentColor;\n";
    if (settings.getScreenSpaceReflections()) {
      shader_src += "layout(location = 1) out vec3 viewSpaceNormal; \n";
    }
    shader_src += "in vec3 pos_world;\n\
in vec2 uv_out;\n\
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
uniform mat3 " + std::string(modelMatrixInverse()) + ";\n\
uniform mat3 " + std::string(modelViewInverse()) + ";\n\
uniform vec4 " + std::string(viewMatrixThirdRow()) + ";\n\
in vec3 normal_local;\n\
in vec3 tangent_local;\n\
in vec3 bitangent_local;\n\
void main()\n\
{\n\
  vec2 uv = uv_out;\n\
  vec3 normal_world = normalize(" + std::string(modelMatrixInverse()) + " * normal_local);\n";
    if (tangent_space) {
      shader_src += "  mat3 world_to_tangent = transpose(mat3(normalize(" + std::string(modelMatrixInverse()) + " * tangent_local), normalize(" + std::string(modelMatrixInverse()) + " * bitangent_local), normal_world));\n";
    }
    shader_src += "  vec3 e =" + std::string(tangent_space ? " world_to_tangent *" : "") + " normalize(" + std::string(cameraPositionWorld()) + " - pos_world); \n";
    if ((flags & MR_NORMAL_MAP) && (flags & MR_HEIGHT_MAP)) { // Parallax only in combination with normal mapping
      if (!settings.getReliefMapping()) {
        shader_src += "  uv -= e.xy / e.z * (1.f - textureLod(" + std::string(heightSampler()) + ", uv, 0.f).r) * " + std::string(parallaxHeightScale()) + "; \n";
      }
      else {
        shader_src += "  float steps = mix(" + std::string(parallaxMaxSteps()) + ", " + std::string(parallaxMinSteps()) + ", clamp(dot(vec3(0.f, 0.f, 1.f), e), 0.f, 1.f));\n\
  vec2 ray = e.xy * " + std::string(parallaxHeightScale()) + ";\n\
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
    if (flags & MeshRenderFlag::MR_ALPHA_MAP) {
      shader_src += "  if (texture(" + std::string(alphaSampler()) + ", uv).r < 0.5) {\n\
    discard;\n\
    return;\n\
  }\n";
    }
    shader_src += "  vec3 l = " + std::string((((flags & MR_HEIGHT_MAP) || (flags & MR_NORMAL_MAP)) ? "world_to_tangent *" : "")) + " normalize(" + std::string(lightPositionWorld()) + " - pos_world);\n";
    if (flags & MeshRenderFlag::MR_NORMAL_MAP) {
      shader_src += "  vec3 normal_ts = normalize((texture(" + std::string(normalSampler()) + ", uv).xyz * 2.f - 1.f));\n\
  float diffuse = clamp(dot(l, normal_ts), 0.f, 1.f);\n\
  float specular = pow(clamp(dot(normalize(e + l), normal_ts), 0.f, 1.f), " + std::string(specularExponent()) + ");\n";
    }
    else {
      shader_src += "  float diffuse = clamp(dot(l, normal_world), 0.f, 1.f);\n\
  float specular = pow(clamp(dot(normalize(e + l), normal_world), 0.f, 1.f), " + std::string(specularExponent()) + ");\n";
    }

    if (flags & MeshRenderFlag::MR_DIFFUSE_MAP) {
      shader_src += "  vec3 albedo = texture(" + std::string(diffuseSampler()) + ", uv).rgb;\n";
    }
    else {
      shader_src += "  vec3 albedo = " + std::string(diffuseColor()) + ";\n";
    }
    if (settings.gammaEnabled()) {
      shader_src += "  albedo = pow(albedo, vec3(" + std::string(gamma()) + "));\n";
    }
    shader_src += "  fragmentColor = I_in * albedo * (" + std::string(ambientConstant()) + " + " + std::string(diffuseConstant()) + " * diffuse + " + std::string(specularConstant()) + " * specular);\n";
    if (settings.getShadows() || settings.getShadowsPCF()) {
      shader_src += "  int index = " + std::string(numfrustumSplits()) + "-1;\n\
  for (int i = " + std::string(numfrustumSplits()) + "-2; i >= 0; i--) {\n\
    index -= int(-dot(" + std::string(viewMatrixThirdRow()) + ", vec4(pos_world, 1.f)) < " + std::string(frustumSplits()) + "[i]);\n\
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
    if (settings.getScreenSpaceReflections()) {
      if (flags & MR_REFLECTIVE) {
        if (tangent_space) {
          shader_src += "  mat3 tangent_to_view = mat3(normalize(" + std::string(modelViewInverse()) + " * tangent_local), normalize(" + std::string(modelViewInverse()) + " * bitangent_local), normalize(" + std::string(modelViewInverse()) + " * normal_local));\n\
  viewSpaceNormal = normalize(tangent_to_view * normal_ts);\n";
        }
        else {
          shader_src += "  viewSpaceNormal = normalize(" + std::string(modelViewInverse()) + " * normal_local);\n";
        }
      }
      else {
        shader_src += "  viewSpaceNormal = vec3(0.f);\n";
      }
    }
    shader_src += "}\n";
    return shader_src;
  }
  std::string GLSLShaderGenerator::createMeshFragmentDepthSource(unsigned flags, const GraphicsSettings & settings) const
  {
    std::string shader_src = "#version 330\n\
in vec2 uv_out;\n";
    if (flags & MR_ALPHA_MAP) {
      shader_src += "uniform sampler2D " + std::string(alphaSampler()) + ";\n";
    }
    shader_src += "void main()\n\
{\n";
    if (flags & MR_ALPHA_MAP) {
      shader_src += "  if (texture(" + std::string(alphaSampler()) + ", uv_out).r < 0.5f){\n\
    discard;\n\
  }\n";
    }
    shader_src += "}";
    return shader_src;
  }
  std::string GLSLShaderGenerator::createCompositeShaderSource(const GraphicsSettings & gs) const
  {
    std::string shader_src = "#version 330\n\
layout(location = 0) out vec3 fragmentColor;\n\
uniform sampler2D " + std::string(lightingSampler()) + ";\n\
uniform sampler2D " + std::string(depthSampler()) + ";\n\
uniform sampler2D " + std::string(dofSampler()) + ";\n\
uniform mat4 " + std::string(projectionMatrixInverse()) + ";\n\
in vec2 uv;\n\
void main()\n\
{\n\
  fragmentColor = textureLod(" + std::string(lightingSampler()) + ", uv, 0.f).rgb;\n";
    if (gs.getDepthOfField()) {
      shader_src += "  vec4 pos_ndc = vec4(vec3(uv, texture(" + std::string(depthSampler()) + ", uv).r) * 2.f - 1.f, 1.f);\n\
  vec4 pos_view_h = " + std::string(projectionMatrixInverse()) + " * pos_ndc;\n\
  float depth_view = -(pos_view_h.z / pos_view_h.w);\n\
  vec3 blur_color = texture(" + std::string(dofSampler()) + ", uv).rgb;\n\
  if (depth_view >= " + std::to_string(gs.getDofCenter()) + "){\n\
    fragmentColor = mix(fragmentColor, blur_color, smoothstep(" + std::to_string(gs.getDofCenter()) + ", " + std::to_string(gs.getDofFar()) + ", depth_view));\n\
  }\n\
  else {\n\
    fragmentColor = mix(blur_color, fragmentColor, smoothstep(" + std::to_string(gs.getDofNear()) + ", " + std::to_string(gs.getDofCenter()) + ", depth_view));\n\
  }\n";
    }
    if (gs.exposureEnabled()) {
      shader_src += "  fragmentColor *= " + std::to_string(gs.getExposure()) + ";\n";
    }
    shader_src += "  fragmentColor /= 1.f + fragmentColor;\n";
    if (gs.gammaEnabled()) {
      shader_src += "  fragmentColor = pow(fragmentColor, vec3(" + std::to_string(1.f / gs.getGamma()) + "));\n";
    }
    return shader_src + "}\n";
  }
}