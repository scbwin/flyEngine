#include <opengl/OpenGLAPI.h>
#include <iostream>
#include <opengl/GLVertexArray.h>
#include <opengl/GLBuffer.h>
#include <Model.h>
#include <Vertex.h>
#include <Mesh.h>
#include <Timing.h>
#include <SOIL/SOIL.h>
#include <Material.h>
#include <Settings.h>
#include <WindParamsLocal.h>
#include <GraphicsSettings.h>
#include <opengl/GLMaterialSetup.h>
#include <fstream>
#include <Flags.h>
#include <ShaderDesc.h>
#include <GlobalShaderParams.h>
#include <math/MathHelpers.h>

#define INIT_BUFFER_SIZE 1024 * 1024 * 8 // Allocate 8 MB video RAM for the vertex and index buffer each.

namespace fly
{
  OpenGLAPI::OpenGLAPI(const Vec4f& clear_color) :
    _vboAABB(GL_ARRAY_BUFFER),
    _boxShader(createMiscShader(GLShaderSource("assets/opengl/vs_box.glsl", GL_VERTEX_SHADER), 
      GLShaderSource("assets/opengl/fs_box.glsl", GL_FRAGMENT_SHADER), GLShaderSource("assets/opengl/gs_box.glsl", GL_GEOMETRY_SHADER))),
    _cullingShader(createComputeShader(GLShaderSource("assets/opengl/cs_culling.glsl", GL_COMPUTE_SHADER))),
    _lodShader(createComputeShader(GLShaderSource("assets/opengl/cs_lod.glsl", GL_COMPUTE_SHADER))),
    _debugFrustumShader(createMiscShader(GLShaderSource("assets/opengl/vs_debug_frustum.glsl", GL_VERTEX_SHADER), GLShaderSource("assets/opengl/fs_debug_frustum.glsl", GL_FRAGMENT_SHADER))),
    _skydomeShader(createMiscShader(GLShaderSource("assets/opengl/vs_skybox.glsl", GL_VERTEX_SHADER), GLShaderSource("assets/opengl/fs_skydome_new.glsl", GL_FRAGMENT_SHADER)))
  {
    GL_CHECK(glGetIntegerv(GL_MAJOR_VERSION, &_glVersionMajor));
    GL_CHECK(glGetIntegerv(GL_MINOR_VERSION, &_glVersionMinor));
    std::cout << ", GL Version: " << _glVersionMajor << "." << _glVersionMinor << std::endl;
    _vaoAABB.bind();
    _vboAABB.bind();
    for (unsigned i = 0; i < 2; i++) {
      GL_CHECK(glEnableVertexAttribArray(i));
      GL_CHECK(glVertexAttribDivisor(i, 1));
    }
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, false, 2 * sizeof(Vec3f), 0));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, false, 2 * sizeof(Vec3f), reinterpret_cast<const void*>(sizeof(Vec3f))));

    GL_CHECK(glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]));
  }
  OpenGLAPI::~OpenGLAPI()
  {
  }
  ZNearMapping OpenGLAPI::getZNearMapping() const
  {
    return ZNearMapping::MINUS_ONE;
  }
  void OpenGLAPI::setViewport(const Vec2u & size) const
  {
    GL_CHECK(glViewport(0, 0, size[0], size[1]));
  }
  std::vector<OpenGLAPI::IndirectInfo> OpenGLAPI::indirectFromMeshData(const std::vector<MeshData>& mesh_data) const
  {
    std::vector<IndirectInfo> infos;
    for (const auto& d : mesh_data) {
      infos.push_back(IndirectInfo(d));
    }
    return infos;
  }
  void OpenGLAPI::beginFrame() const
  {
      for (unsigned i = 0; _anisotropy > 1 && i <= static_cast<unsigned>(heightTexUnit); i++) {
        _samplerAnisotropic.bind(i);
      }
  }
  void OpenGLAPI::bindShader(GLShaderProgram const * shader)
  {
    _activeShader = shader;
    _activeShader->bind();
  }
  void OpenGLAPI::bindShadowmap(const Shadowmap & shadowmap) const
  {
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + miscTexUnit0));
    shadowmap.bind();
  }
  void OpenGLAPI::renderMesh(const MeshData & mesh_data) const
  {
    GL_CHECK(glDrawElementsBaseVertex(GL_TRIANGLES, mesh_data._count, mesh_data._type, mesh_data._indices, mesh_data._baseVertex));
  }
  void OpenGLAPI::renderMesh(const MeshData & mesh_data, const Mat4f & model_matrix) const
  {
    setMatrix(_activeShader->uniformLocation(GLSLShaderGenerator::modelMatrix), model_matrix);
    renderMesh(mesh_data);
  }
  void OpenGLAPI::renderMesh(const MeshData& mesh_data, const Mat4f& model_matrix, const Mat3f& model_matrix_inverse) const
  {
    setMatrixTranspose(_activeShader->uniformLocation(GLSLShaderGenerator::modelMatrixInverse), model_matrix_inverse);
    renderMesh(mesh_data, model_matrix);
  }
  void OpenGLAPI::renderMesh(const MeshData & mesh_data, const Mat4f & model_matrix, const WindParamsLocal & params, const AABB & aabb) const
  {
    setScalar(_activeShader->uniformLocation(GLSLShaderGenerator::windPivot), params._pivotWorld);
    setScalar(_activeShader->uniformLocation(GLSLShaderGenerator::windExponent), params._bendFactorExponent);
    setVector(_activeShader->uniformLocation(GLSLShaderGenerator::bbMin), aabb.getMin());
    setVector(_activeShader->uniformLocation(GLSLShaderGenerator::bbMax), aabb.getMax());
    renderMesh(mesh_data, model_matrix);
  }
  void OpenGLAPI::renderMesh(const MeshData & mesh_data, const Mat4f & model_matrix, const Mat3f & model_matrix_inverse, const WindParamsLocal& params, const AABB& aabb) const
  {
    setMatrixTranspose(_activeShader->uniformLocation(GLSLShaderGenerator::modelMatrixInverse), model_matrix_inverse);
    renderMesh(mesh_data, model_matrix, params, aabb);
  }
  void OpenGLAPI::renderMesh(const MeshData & mesh_data, const Mat4f & model_matrix, const WindParamsLocal & wind_params, const Sphere & sphere) const
  {
    renderMesh(mesh_data, model_matrix, wind_params, AABB(sphere.getMin(), sphere.getMax()));
  }
  void OpenGLAPI::renderMesh(const MeshData & mesh_data, const Mat4f & model_matrix, const Mat3f & model_matrix_inverse, const WindParamsLocal & wind_params, const Sphere & sphere) const
  {
    renderMesh(mesh_data, model_matrix, model_matrix_inverse, wind_params, AABB(sphere.getMin(), sphere.getMax()));
  }
  void OpenGLAPI::renderMeshMVP(const MeshData & mesh_data, const Mat4f & mvp) const
  {
    setMatrix(_activeShader->uniformLocation(GLSLShaderGenerator::modelViewProjectionMatrix), mvp);
    renderMesh(mesh_data);
  }
  void OpenGLAPI::renderBVs(const StackPOD<AABB const *>& aabbs, const Mat4f& transform, const Vec3f& col)
  {
    _vaoAABB.bind();
    bindShader(&_boxShader);
    setMatrix(_activeShader->uniformLocation(GLSLShaderGenerator::viewProjectionMatrix), transform);
    setVector(_activeShader->uniformLocation("c"), col);
    StackPOD<Vec3f> bb_buffer;
    bb_buffer.reserve(aabbs.size() * 2u);
    for (const auto& aabb : aabbs) {
      bb_buffer.push_back(aabb->getMin());
      bb_buffer.push_back(aabb->getMax());
    }
    _vboAABB.setData(bb_buffer.begin(), bb_buffer.size(), GL_DYNAMIC_COPY);
    GL_CHECK(glDrawArraysInstanced(GL_POINTS, 0, 1, static_cast<GLsizei>(aabbs.size())));
  }
  void OpenGLAPI::renderBVs(const StackPOD<Sphere const *>& spheres, const Mat4f& transform, const Vec3f& col)
  {

  }
  void OpenGLAPI::renderDebugFrustum(const Mat4f & vp_debug_frustum, const Mat4f & vp)
  {
    GL_CHECK(glLineWidth(4.f));
    bindShader(&_debugFrustumShader);
    auto mat = vp * inverse(vp_debug_frustum);
    auto cube_ndc = MathHelpers::cubeNDC(getZNearMapping());
    setMatrix(_activeShader->uniformLocation("t"), mat);
    GLVertexArray vao;
    vao.bind();
    GLBuffer vbo(GL_ARRAY_BUFFER);
    vbo.setData(cube_ndc.data(), cube_ndc.size());
    GLBuffer ibo(GL_ELEMENT_ARRAY_BUFFER);
    std::array<unsigned short, 24> indices = { 0, 1, 1, 3, 3, 2, 2, 0, 0, 4, 4, 5, 5, 1, 3, 7, 7, 5, 7, 6, 6, 4, 6, 2 };
    ibo.setData(indices.data(), indices.size());
    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vec3f), 0));
    GL_CHECK(glDrawElements(GL_LINES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_SHORT, nullptr));
    GL_CHECK(glLineWidth(1.f));
  }
  void OpenGLAPI::prepareCulling(const std::array<Vec4f, 6>& frustum_planes, const Vec3f& cam_pos_world, float lod_range, float thresh)
  {
    bindShader(&_cullingShader);
    setVectorArray(_activeShader->uniformLocation("fp"), frustum_planes.front(), static_cast<unsigned>(frustum_planes.size()));
    setVector(_activeShader->uniformLocation("cp_w"), cam_pos_world);
    setScalar(_activeShader->uniformLocation("lr"), lod_range);
    setScalar(_activeShader->uniformLocation("de"), thresh);
  }
  void OpenGLAPI::prepareLod(const Vec3f & cam_pos_world, float lod_range, float thresh)
  {
    bindShader(&_lodShader);
    setVector(_activeShader->uniformLocation("cp_w"), cam_pos_world);
    setScalar(_activeShader->uniformLocation("lr"), lod_range);
    setScalar(_activeShader->uniformLocation("de"), thresh);
  }
  void OpenGLAPI::endCulling() const
  {
    GL_CHECK(glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT));
  }
  void OpenGLAPI::cullInstances(const StorageBuffer& aabb_buffer, unsigned num_instances,
    const StorageBuffer& visible_instances, const IndirectBuffer& indirect_draw_buffer,
    std::vector<IndirectInfo>& info) const
  {
    for (auto& i : info) {
      i._primCount = 0;
    }
    indirect_draw_buffer.setData(info.data(), info.size(), GL_DYNAMIC_DRAW);

    unsigned group_size = 1024; // TODO: remove hard-coded value
    unsigned num_groups = static_cast<unsigned>(std::ceil(static_cast<float>(num_instances) / static_cast<float>(group_size)));

    aabb_buffer.bindBase(GLSLShaderGenerator::bufferBindingAABB);
    visible_instances.bindBase(GLSLShaderGenerator::bufferBindingVisibleInstances);
    indirect_draw_buffer.bindBase(GL_SHADER_STORAGE_BUFFER, GLSLShaderGenerator::bufferBindingIndirectInfo);

    setScalar(_activeShader->uniformLocation("ni"), num_instances);
    setScalar(_activeShader->uniformLocation("ml"), static_cast<unsigned>(info.size() - 1));

    GL_CHECK(glDispatchCompute(num_groups, 1, 1));
    
   /*
   GL_CHECK(glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT));
    auto draw_indirect_ptr = indirect_draw_buffer.map<IndirectInfo>(GL_READ_ONLY);
    for (unsigned i = 0; i < info.size(); i++) {
      auto num_visible_instances = draw_indirect_ptr[i]._primCount;
      std::cout << "Visible instances lod " << i << ":" << num_visible_instances << std::endl;
    }
    indirect_draw_buffer.unmap();*/
  }
  void OpenGLAPI::renderInstances(const StorageBuffer & visible_instances, const IndirectBuffer & indirect_draw_buffer,
    const StorageBuffer & instance_data, const std::vector<IndirectInfo>& info, unsigned num_instances) const
  {
    instance_data.bindBase(GLSLShaderGenerator::bufferBindingInstanceData);
    visible_instances.bindBase(GLSLShaderGenerator::bufferBindingVisibleInstances);
    indirect_draw_buffer.bind(GL_DRAW_INDIRECT_BUFFER);
    for (unsigned i = 0; i < info.size(); i++) {
      setScalar(_activeShader->uniformLocation("offs"), num_instances * i);
      GL_CHECK(glDrawElementsIndirect(GL_TRIANGLES, info[i]._type, reinterpret_cast<void*>(i * sizeof(IndirectInfo))));
    }
  }
  void OpenGLAPI::setRendertargets(const RendertargetStack& rtts, const Depthbuffer* depth_buffer)
  {
    setColorBuffers(rtts);
    _offScreenFramebuffer.texture(GL_DEPTH_ATTACHMENT, depth_buffer, 0);
    checkFramebufferStatus();
  }
  void OpenGLAPI::setRendertargets(const RendertargetStack& rtts, const Depthbuffer* depth_buffer, unsigned depth_buffer_layer)
  {
    setColorBuffers(rtts);
    _offScreenFramebuffer.textureLayer(GL_DEPTH_ATTACHMENT, depth_buffer, 0, depth_buffer_layer);
    checkFramebufferStatus();
  }
  void OpenGLAPI::bindBackbuffer(unsigned id) const
  {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, id));
  }
  void OpenGLAPI::ssr(const RTT & lighting_buffer, const RTT& view_space_normals, const Depthbuffer& depth_buffer, const Mat4f& projection_matrix, const Vec4f& blend_weight, RTT& lighting_buffer_copy)
  {
    lighting_buffer_copy = lighting_buffer;
    lighting_buffer_copy.param(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    lighting_buffer_copy.param(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    depth_buffer.param(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    depth_buffer.param(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    GL_CHECK(glGenerateMipmap(depth_buffer.target()));
    GL_CHECK(glEnable(GL_BLEND));
    GL_CHECK(glBlendColor(blend_weight[0], blend_weight[1], blend_weight[2], blend_weight[3]));
    GL_CHECK(glBlendFunc(GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_COLOR));
    bindShader(&_ssrShader);
    activateTexture(view_space_normals, GLSLShaderGenerator::viewSpaceNormalsSampler, miscTexUnit0);
    activateTexture(lighting_buffer_copy, GLSLShaderGenerator::lightingSampler, miscTexUnit1);
    activateTexture(depth_buffer, GLSLShaderGenerator::depthSampler, miscTexUnit2);
    setMatrix(_ssrShader.uniformLocation(GLSLShaderGenerator::projectionMatrix), projection_matrix);
    auto p_inverse = inverse(projection_matrix);
    setMatrix(_ssrShader.uniformLocation(GLSLShaderGenerator::projectionMatrixInverse), p_inverse);
    setVector(_ssrShader.uniformLocation(GLSLShaderGenerator::projectionMatrixInverseThirdRow), p_inverse.row(2));
    setVector(_ssrShader.uniformLocation(GLSLShaderGenerator::projectionMatrixInverseFourthRow), p_inverse.row(3));
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
    GL_CHECK(glDisable(GL_BLEND));
    depth_buffer.param(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    depth_buffer.param(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  }
  void OpenGLAPI::separableBlur(const RTT & in, const std::array<std::shared_ptr<RTT>, 2>& out, OpenGLAPI::RendertargetStack& rtt_stack)
  {
    in.param(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    bindShader(&_blurShader);
    _activeShader->bind();
    for (unsigned i = 0; i < 2; i++) {
      rtt_stack.clear();
      rtt_stack.push_back_secure(out[!i].get());
      setRendertargets(rtt_stack, nullptr);
      Vec2f texel_size(1.f / out[0]->width() * i, 1.f / out[0]->height() * !i);
      setVector(_activeShader->uniformLocation(GLSLShaderGenerator::texelSize), texel_size);
      activateTexture(i ? *out[1] : in, GLSLShaderGenerator::toBlurSampler, miscTexUnit1);
      GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
    }
    in.param(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  }
  void OpenGLAPI::renderGodRays(const OpenGLAPI::Depthbuffer & depth_buffer, const RTT& lighting_buffer, const Vec2f & light_pos_uv)
  {
    bindShader(&_godRayShader);
    depth_buffer.param(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    activateTexture(depth_buffer, GLSLShaderGenerator::depthSampler, miscTexUnit0);
    activateTexture(lighting_buffer, GLSLShaderGenerator::lightingSampler, miscTexUnit1);
    setVector(_activeShader->uniformLocation(GLSLShaderGenerator::lightPosUV), light_pos_uv);
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
    depth_buffer.param(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  }
  void OpenGLAPI::composite(const RTT& lighting_buffer, const GlobalShaderParams& params)
  {
    bindShader(&_compositeShader);
    activateTexture(lighting_buffer, GLSLShaderGenerator::lightingSampler, miscTexUnit1);
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
  }
  void OpenGLAPI::composite(const RTT & lighting_buffer, const GlobalShaderParams & params, const RTT & dof_buffer, const Depthbuffer& depth_buffer)
  {
    bindShader(&_compositeShader);
    auto p_inverse = inverse(params._projectionMatrix);
    setVector(_activeShader->uniformLocation(GLSLShaderGenerator::projectionMatrixInverseThirdRow), p_inverse.row(2));
    setVector(_activeShader->uniformLocation(GLSLShaderGenerator::projectionMatrixInverseFourthRow), p_inverse.row(3));
    activateTexture(lighting_buffer, GLSLShaderGenerator::lightingSampler, miscTexUnit1);
    activateTexture(dof_buffer, GLSLShaderGenerator::dofSampler, miscTexUnit2);
    activateTexture(depth_buffer, GLSLShaderGenerator::depthSampler, miscTexUnit3);
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
  }
  void OpenGLAPI::composite(const RTT & lighting_buffer, const GlobalShaderParams & params, const RTT & dof_buffer, 
    const Depthbuffer & depth_buffer, const RTT & god_ray_buffer, const Vec3f& god_ray_intensity)
  {
    bindShader(&_compositeShader);
    auto p_inverse = inverse(params._projectionMatrix);
    setVector(_activeShader->uniformLocation(GLSLShaderGenerator::projectionMatrixInverseThirdRow), p_inverse.row(2));
    setVector(_activeShader->uniformLocation(GLSLShaderGenerator::projectionMatrixInverseFourthRow), p_inverse.row(3));
    activateTexture(lighting_buffer, GLSLShaderGenerator::lightingSampler, miscTexUnit1);
    activateTexture(dof_buffer, GLSLShaderGenerator::dofSampler, miscTexUnit2);
    activateTexture(depth_buffer, GLSLShaderGenerator::depthSampler, miscTexUnit3);
    activateTexture(god_ray_buffer, GLSLShaderGenerator::godRaySampler, miscTexUnit4);
    setVector(_activeShader->uniformLocation(GLSLShaderGenerator::godRayIntensity), god_ray_intensity);
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
  }
  void OpenGLAPI::endFrame() const
  {
    for (unsigned i = 0; i <= static_cast<unsigned>(heightTexUnit); i++) {
      _samplerAnisotropic.unbind(i);
    }
  }
  void OpenGLAPI::setAnisotropy(unsigned anisotropy)
  {
    if (_glVersionMajor >= 4 && _glVersionMinor >= 6) {
      float max_ani;
      GL_CHECK(glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_ani));
      _anisotropy = glm::clamp(anisotropy, 1u, static_cast<unsigned>(max_ani));
      _samplerAnisotropic.param(GL_TEXTURE_MAX_ANISOTROPY, static_cast<float>(_anisotropy));
    }
  }
  void OpenGLAPI::enablePolygonOffset(float factor, float units) const
  {
    GL_CHECK(glEnable(GL_POLYGON_OFFSET_FILL));
    GL_CHECK(glPolygonOffset(factor, units));
  }
  void OpenGLAPI::disablePolygonOffset() const
  {
    GL_CHECK(glDisable(GL_POLYGON_OFFSET_FILL));
  }
  void OpenGLAPI::renderSkydome(const Mat4f & view_projection_matrix, const MeshData& mesh_data)
  {
    bindShader(&_skydomeShader);
    setMatrix(_activeShader->uniformLocation(GLSLShaderGenerator::viewProjectionMatrix), view_projection_matrix);
    renderMesh(mesh_data);
  }
  OpenGLAPI::Texture* OpenGLAPI::createTexture(const std::string & path)
  {
    auto tex = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_COMPRESS_TO_DXT);
    if (tex) {
      return new OpenGLAPI::Texture(tex, GL_TEXTURE_2D);
    }
    else {
      throw std::exception((std::string("Could not create texture ") + path).c_str());
      return nullptr;
    }
  }
  OpenGLAPI::Shader* OpenGLAPI::createShader(OpenGLAPI::ShaderSource& vs, OpenGLAPI::ShaderSource& fs, OpenGLAPI::ShaderSource& gs)
  {
    auto ret = new Shader();
    ret->add(vs);
    if (gs._key != "") { ret->add(gs); }
    ret->add(fs);
    ret->link();
    return ret;
  }
  OpenGLAPI::Shader OpenGLAPI::createComputeShader(OpenGLAPI::ShaderSource & source)
  {
    OpenGLAPI::Shader program;
    program.add(source);
    program.link();
    return program;
  }
  std::unique_ptr<OpenGLAPI::RTT> OpenGLAPI::createRenderToTexture(const Vec2u & size, OpenGLAPI::TexFilter filter)
  {
    auto tex = std::make_unique<GLTexture>(GL_TEXTURE_2D);
    tex->bind();
    auto tex_filter = filter == TexFilter::NEAREST ? GL_NEAREST : GL_LINEAR;
    tex->param(GL_TEXTURE_MIN_FILTER, tex_filter);
    tex->param(GL_TEXTURE_MAG_FILTER, tex_filter);
    tex->param(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    tex->param(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    tex->image2D(0, GL_RGBA16F, size, 0, GL_RGBA, GL_FLOAT, nullptr);
    return tex;
  }
  std::unique_ptr<OpenGLAPI::Depthbuffer> OpenGLAPI::createDepthbuffer(const Vec2u & size)
  {
    auto tex = std::make_unique<GLTexture>(GL_TEXTURE_2D);
    tex->bind();
    tex->param(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    tex->param(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    tex->param(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    tex->param(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    tex->image2D(0, GL_DEPTH_COMPONENT24, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    return tex;
  }
  std::unique_ptr<OpenGLAPI::Shadowmap> OpenGLAPI::createShadowmap(const GraphicsSettings& settings)
  {
    auto tex = std::make_unique<GLTexture>(GL_TEXTURE_2D_ARRAY);
    tex->bind();
    GLint filter = settings.getShadowsPCF() ? GL_LINEAR : GL_NEAREST;
    tex->param(GL_TEXTURE_MIN_FILTER, filter);
    tex->param(GL_TEXTURE_MAG_FILTER, filter);
    tex->param(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    tex->param(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    tex->param(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    tex->param(GL_TEXTURE_BORDER_COLOR, Vec4f(std::numeric_limits<float>::max()).ptr());
    if (settings.getShadowsPCF()) {
      tex->param(GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
      tex->param(GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
    }
    resizeShadowmap(tex.get(), settings);
    return tex;
  }
  OpenGLAPI::IndirectBuffer OpenGLAPI::createIndirectBuffer(const IndirectInfo & info) const
  {
    IndirectBuffer buffer(GL_DRAW_INDIRECT_BUFFER);
    buffer.setData(&info, 1);
    return buffer;
  }
  OpenGLAPI::IndirectBuffer OpenGLAPI::createIndirectBuffer(const std::vector<IndirectInfo>& info) const
  {
    IndirectBuffer buffer(GL_DRAW_INDIRECT_BUFFER);
    buffer.setData(info.data(), info.size());
    return buffer;
  }
  void OpenGLAPI::resizeShadowmap(Shadowmap* shadow_map, const GraphicsSettings& settings)
  {
    shadow_map->image3D(0, GL_DEPTH_COMPONENT24, Vec3u(Vec2u(settings.getShadowMapSize()), static_cast<unsigned>(settings.getFrustumSplits().size())), 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  }
  void OpenGLAPI::createBlurShader(const GraphicsSettings & gs)
  {
    GLShaderSource vs, fs;
    _shaderGenerator.createBlurShaderSource(gs, vs, fs);
    _blurShader = std::move(createMiscShader(vs, fs));
  }
  void OpenGLAPI::createCompositeShader(const GraphicsSettings & gs)
  {
    GLShaderSource vs, fs;
    _shaderGenerator.createCompositeShaderSource(gs, vs, fs);
    _compositeShader = std::move(createMiscShader(vs, fs));
  }
  void OpenGLAPI::createScreenSpaceReflectionsShader(const GraphicsSettings & gs)
  {
    GLShaderSource vs, fs;
    _shaderGenerator.createSSRShaderSource(gs, vs, fs);
    _ssrShader = std::move(createMiscShader(vs, fs));
  }
  void OpenGLAPI::createGodRayShader(const GraphicsSettings & gs)
  {
    ShaderSource vs, fs;
    _shaderGenerator.createGodRayShaderSource(gs, vs, fs);
    _godRayShader = std::move(createMiscShader(vs, fs));
  }
  //const std::shared_ptr<ShaderDesc<OpenGLAPI>>& OpenGLAPI::getSkydomeShaderDesc() const
  //{
  //  return _skydomeShaderDesc;
  //}
  const OpenGLAPI::ShaderGenerator& OpenGLAPI::getShaderGenerator() const
  {
    return _shaderGenerator;
  }
  OpenGLAPI::Shader const *& OpenGLAPI::getActiveShader()
  {
    return _activeShader;
  }
  void OpenGLAPI::checkFramebufferStatus()
  {
    if (GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      std::cout << "Framebuffer imcomplete" << std::endl;
    }
  }
  void OpenGLAPI::setColorBuffers(const RendertargetStack& rtts)
  {
    _offScreenFramebuffer.bind();
    _offScreenFramebuffer.clearAttachments();
    _drawBuffers.clear();
    if (rtts.size()) {
      for (unsigned i = 0; i < rtts.size(); i++) {
        _drawBuffers.push_back_secure(GL_COLOR_ATTACHMENT0 + i);
        _offScreenFramebuffer.texture(_drawBuffers.back(), rtts[i], 0);
      }
    }
    else {
      _drawBuffers.push_back_secure(GL_NONE);
    }
    GL_CHECK(glDrawBuffers(static_cast<GLsizei>(_drawBuffers.size()), _drawBuffers.begin()));
  }
  OpenGLAPI::Shader OpenGLAPI::createMiscShader(OpenGLAPI::ShaderSource & vs, OpenGLAPI::ShaderSource & fs, OpenGLAPI::ShaderSource& gs) const
  {
    Shader shader;
    shader.add(vs);
    if (gs._key != "") {
      shader.add(gs);
    }
    shader.add(fs);
    shader.link();
    return shader;
  }
  OpenGLAPI::MeshGeometryStorage::MeshGeometryStorage() :
    _vboStack(GL_ARRAY_BUFFER, INIT_BUFFER_SIZE),
    _iboStack(GL_ELEMENT_ARRAY_BUFFER, INIT_BUFFER_SIZE),
    _meshDataCache(SoftwareCache<std::shared_ptr<Mesh>, MeshData, const std::shared_ptr<Mesh>&>([this](
      const std::shared_ptr<Mesh>& mesh) {
    MeshData mesh_data;
    mesh_data._count = static_cast<GLsizei>(mesh->getIndices().size());
    mesh_data._baseVertex = static_cast<GLint>(_baseVertex);
    mesh_data._indices = reinterpret_cast<GLvoid*>(_indices);
    _baseVertex += mesh->getVertices().size();
    _vboStack.push_back(mesh->getVertices().data(), mesh->getVertices().size());
    if (mesh->getVertices().size() - 1 <= static_cast<size_t>(std::numeric_limits<unsigned short>::max())) {
      StackPOD<unsigned short> indices(mesh->getIndices().size());
      for (unsigned i = 0; i < indices.size(); i++) {
        indices[i] = static_cast<unsigned short>(mesh->getIndices()[i]);
      }
      _indices += indices.size() * sizeof(*indices.begin());
      _iboStack.push_back(indices.begin(), indices.size());
      mesh_data._type = GL_UNSIGNED_SHORT;
    }
    else {
      _indices += mesh->getIndices().size() * sizeof(mesh->getIndices().front());
      _iboStack.push_back(mesh->getIndices().data(), mesh->getIndices().size());
      mesh_data._type = GL_UNSIGNED_INT;
    }
    _vao.bind();
    _vboStack.getBuffer().bind();
    _iboStack.getBuffer().bind();
    for (unsigned i = 0; i < 5; i++) {
      GL_CHECK(glEnableVertexAttribArray(i));
    }
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _position))));
    GL_CHECK(glVertexAttribPointer(1, 4, GL_INT_2_10_10_10_REV, GL_TRUE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _normal))));
    GL_CHECK(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _uv))));
    GL_CHECK(glVertexAttribPointer(3, 4, GL_INT_2_10_10_10_REV, GL_TRUE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _tangent))));
    GL_CHECK(glVertexAttribPointer(4, 4, GL_INT_2_10_10_10_REV, GL_TRUE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _bitangent))));
    return mesh_data;
  }))
  {
  }
  OpenGLAPI::MeshGeometryStorage::~MeshGeometryStorage()
  {
  }
  void OpenGLAPI::MeshGeometryStorage::bind() const
  {
    _vao.bind();
  }
  OpenGLAPI::MeshData OpenGLAPI::MeshGeometryStorage::addMesh(const std::shared_ptr<Mesh>& mesh)
  {
    return _meshDataCache.getOrCreate(mesh, mesh);
  }
  OpenGLAPI::GlewInit::GlewInit()
  {
    glewExperimental = true;
    auto result = glewInit();
    if (result == GLEW_OK) {
      std::cout << "OpenGLAPI::OpenGLAPI(): Initialized GLEW";
    }
    else {
      std::cout << "OpenGLAPI::OpenGLAPI() Failed to initialized GLEW: " << glewGetErrorString(result) << std::endl;
    }
  }
}