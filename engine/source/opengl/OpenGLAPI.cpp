#include <opengl/OpenGLAPI.h>
#include <iostream>
#include <opengl/GLVertexArray.h>
#include <opengl/GLBuffer.h>
#include <Model.h>
#include <Vertex.h>
#include <Mesh.h>
#include <opengl/GLWrappers.h>
#include <Timing.h>
#include <StaticModelRenderable.h>
#include <SOIL/SOIL.h>
#include <opengl/GLAppendBuffer.h>
#include <Material.h>
#include <opengl/GLShaderInterface.h>
#include <opengl/GLFramebuffer.h>
#include <opengl/GLSLShaderGenerator.h>
#include <Settings.h>
#include <renderer/RenderParams.h>
#include <opengl/GLSampler.h>
#include <WindParamsLocal.h>
#include <GraphicsSettings.h>
#include <opengl/GLMaterialSetup.h>
#include <opengl/GLShaderProgram.h>
#include <fstream>

namespace fly
{
  OpenGLAPI::OpenGLAPI() :
    _shaderGenerator(std::make_unique<GLSLShaderGenerator>()),
    _shaderCache(SoftwareCache<std::string, std::shared_ptr<GLShaderProgram>, GLShaderSource&,
      GLShaderSource&, GLShaderSource& >([](GLShaderSource& vs, GLShaderSource& fs, GLShaderSource& gs) {
    auto ret = std::make_shared<GLShaderProgram>();
    ret->add(vs);
    if (gs._key != "") { ret->add(gs); }
    ret->add(fs);
    ret->link();
    return ret;
  })),
    _textureCache(SoftwareCache<std::string, std::shared_ptr<GLTexture>, const std::string&>([](const std::string& path) {
    auto tex = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_COMPRESS_TO_DXT);
    return tex != 0 ? std::make_shared<GLTexture>(tex, GL_TEXTURE_2D) : nullptr;
  })),
    _matDescCache(SoftwareCache<std::shared_ptr<Material>, std::shared_ptr<MaterialDesc>, const std::shared_ptr<Material>&, const GraphicsSettings&>(
      [this](const std::shared_ptr<Material>& material, const GraphicsSettings&  settings) {
    return std::make_shared<MaterialDesc>(material, this, settings);
  })),
    _shaderDescCache(SoftwareCache<std::shared_ptr<GLShaderProgram>, std::shared_ptr<ShaderDesc>, const std::shared_ptr<GLShaderProgram>&, unsigned>(
      [this](const std::shared_ptr<GLShaderProgram>& shader, unsigned flags) {
    return std::make_shared<ShaderDesc>(shader, flags);
  })),
    _materialSetup(std::make_unique<GLMaterialSetup>())
  {
    glewExperimental = true;
    auto result = glewInit();
    if (result == GLEW_OK) {
      GL_CHECK(glGetIntegerv(GL_MAJOR_VERSION, &_glVersionMajor));
      GL_CHECK(glGetIntegerv(GL_MINOR_VERSION, &_glVersionMinor));
      std::cout << "OpenGLAPI::OpenGLAPI(): Initialized GLEW, GL Version: " << _glVersionMajor << "." << _glVersionMinor << std::endl;
    }
    else {
      std::cout << "OpenGLAPI::OpenGLAPI() Failed to initialized GLEW: " << glewGetErrorString(result) << std::endl;
    }
    _vaoAABB = std::make_shared<GLVertexArray>();
    _vaoAABB->bind();
    _vboAABB = std::make_shared<GLBuffer>(GL_ARRAY_BUFFER);
    _vboAABB->bind();
    for (unsigned i = 0; i < 2; i++) {
      GL_CHECK(glEnableVertexAttribArray(i));
      GL_CHECK(glVertexAttribDivisor(i, 1));
    }
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, false, 2 * sizeof(Vec3f), 0));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, false, 2 * sizeof(Vec3f), reinterpret_cast<const void*>(sizeof(Vec3f))));

    _offScreenFramebuffer = std::make_unique<GLFramebuffer>();

    GLShaderSource vs_aabb, gs_aabb, fs_aabb;
    vs_aabb.initFromFile("assets/opengl/vs_aabb.glsl", GL_VERTEX_SHADER);
    gs_aabb.initFromFile("assets/opengl/gs_aabb.glsl", GL_GEOMETRY_SHADER);
    fs_aabb.initFromFile("assets/opengl/fs_aabb.glsl", GL_FRAGMENT_SHADER);
    _aabbShader = createShader(vs_aabb, fs_aabb, gs_aabb);
    GLShaderSource vs_skydome, fs_skydome;
    vs_skydome.initFromFile("assets/opengl/vs_skybox.glsl", GL_VERTEX_SHADER);
    fs_skydome.initFromFile("assets/opengl/fs_skydome_new.glsl", GL_FRAGMENT_SHADER);
    _skydomeShader = createShader(vs_skydome, fs_skydome);
    _skydomeShaderDesc = createShaderDesc(_skydomeShader, ShaderSetupFlags::VP);

    _samplerAnisotropic = std::make_unique<GLSampler>();
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
  void OpenGLAPI::reloadShaders()
  {
    for (const auto& e : _shaderCache.getElements()) {
     // e->reload();
    }
   // _skydomeShader->reload();
  }
  void OpenGLAPI::beginFrame() const
  {
    if (_anisotropy > 1) {
      for (unsigned i = 0; i <= static_cast<unsigned>(heightTexUnit()); i++) {
        _samplerAnisotropic->bind(i);
      }
    }
  }
  void OpenGLAPI::bindShader(GLShaderProgram * shader)
  {
    _activeShader = shader;
    _activeShader->bind();
  }
  void OpenGLAPI::setupShaderDesc(const ShaderDesc & desc, const GlobalShaderParams& params)
  {
    bindShader(desc.getShader().get());
    desc.setup(params);
  }
  void OpenGLAPI::bindShadowmap(const Shadowmap & shadowmap) const
  {
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + shadowTexUnit()));
    shadowmap.bind();
  }
  void OpenGLAPI::renderMesh(const MeshGeometryStorage::MeshData & mesh_data) const
  {
    GL_CHECK(glDrawElementsBaseVertex(GL_TRIANGLES, mesh_data._count, mesh_data._type, mesh_data._indices, mesh_data._baseVertex));
  }
  void OpenGLAPI::renderMesh(const MeshGeometryStorage::MeshData & mesh_data, const Mat4f & model_matrix) const
  {
    setMatrix(_activeShader->uniformLocation(GLSLShaderGenerator::modelMatrix()), model_matrix);
    renderMesh(mesh_data);
  }
  void OpenGLAPI::renderMesh(const MeshGeometryStorage::MeshData& mesh_data, const Mat4f& model_matrix, const Mat3f& model_matrix_inverse) const
  {
    setMatrixTranspose(_activeShader->uniformLocation(GLSLShaderGenerator::modelMatrixInverse()), model_matrix_inverse);
    renderMesh(mesh_data, model_matrix);
  }
  void OpenGLAPI::renderMesh(const MeshGeometryStorage::MeshData & mesh_data, const Mat4f & model_matrix, const WindParamsLocal & params, const AABB & aabb) const
  {
    setScalar(_activeShader->uniformLocation(GLSLShaderGenerator::windPivot()), params._pivotWorld);
    setScalar(_activeShader->uniformLocation(GLSLShaderGenerator::windExponent()), params._bendFactorExponent);
    setVector(_activeShader->uniformLocation(GLSLShaderGenerator::bbMin()), aabb.getMin());
    setVector(_activeShader->uniformLocation(GLSLShaderGenerator::bbMax()), aabb.getMax());
    renderMesh(mesh_data, model_matrix);
  }
  void OpenGLAPI::renderMesh(const MeshGeometryStorage::MeshData & mesh_data, const Mat4f & model_matrix, const Mat3f & model_matrix_inverse, const WindParamsLocal& params, const AABB& aabb) const
  {
    setMatrixTranspose(_activeShader->uniformLocation(GLSLShaderGenerator::modelMatrixInverse()), model_matrix_inverse);
    renderMesh(mesh_data, model_matrix, params, aabb);
  }

  void OpenGLAPI::renderMeshMVP(const MeshGeometryStorage::MeshData & mesh_data, const Mat4f & mvp) const
  {
    setMatrix(_activeShader->uniformLocation(GLSLShaderGenerator::modelViewProjectionMatrix()), mvp);
    GL_CHECK(glDrawElementsBaseVertex(GL_TRIANGLES, mesh_data._count, mesh_data._type, mesh_data._indices, mesh_data._baseVertex));
  }
  void OpenGLAPI::renderAABBs(const std::vector<AABB const *>& aabbs, const Mat4f& transform, const Vec3f& col)
  {
    _vaoAABB->bind();
    bindShader(_aabbShader.get());
    setMatrix(_activeShader->uniformLocation(GLSLShaderGenerator::viewProjectionMatrix()), transform);
    setVector(_activeShader->uniformLocation("c"), col);
    std::vector<Vec3f> bb_buffer;
    for (const auto& aabb : aabbs) {
      bb_buffer.push_back(aabb->getMin());
      bb_buffer.push_back(aabb->getMax());
    }
    _vboAABB->setData(bb_buffer.data(), bb_buffer.size(), GL_DYNAMIC_COPY);
    GL_CHECK(glDrawArraysInstanced(GL_POINTS, 0, 1, static_cast<GLsizei>(aabbs.size())));
  }
  void OpenGLAPI::setRendertargets(const std::vector<RTT*>& rtts, const Depthbuffer* depth_buffer)
  {
    setColorBuffers(rtts);
    _offScreenFramebuffer->texture(depth_buffer && depth_buffer->format() == GL_DEPTH_STENCIL ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT, depth_buffer, 0);
    checkFramebufferStatus();
  }
  void OpenGLAPI::setRendertargets(const std::vector<RTT*>& rtts, const Depthbuffer* depth_buffer, unsigned depth_buffer_layer)
  {
    setColorBuffers(rtts);
    _offScreenFramebuffer->textureLayer(depth_buffer && depth_buffer->format() == GL_DEPTH_STENCIL ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT, depth_buffer, 0, depth_buffer_layer);
    checkFramebufferStatus();
  }
  void OpenGLAPI::bindBackbuffer(unsigned id) const
  {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, id));
  }
  void OpenGLAPI::separableBlur(const RTT & in, const std::array<std::shared_ptr<RTT>, 2>& out)
  {
    in.param(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    _activeShader = _sepBlurShaderDesc->getShader().get();
    _activeShader->bind();
    setViewport(fly::Vec2u(out[1]->width(), out[1]->height()));
    for (unsigned i = 0; i < 2; i++) {
      setRendertargets({ out[!i].get() }, nullptr);
      Vec2f texel_size(1.f / out[0]->width() * i, 1.f / out[0]->height() * !i);
      setVector(_activeShader->uniformLocation(GLSLShaderGenerator::texelSize()), texel_size);
      GL_CHECK(glActiveTexture(GL_TEXTURE0 + lightingTexUnit()));
      i ? out[1]->bind() : in.bind();
      setScalar(_activeShader->uniformLocation(GLSLShaderGenerator::toBlurSampler()), lightingTexUnit());
      GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
    }
    in.param(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  }
  void OpenGLAPI::composite(const RTT& lighting_buffer, const GlobalShaderParams& params)
  {
    setupShaderDesc(*_compositeShaderDesc, params);
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + lightingTexUnit()));
    lighting_buffer.bind();
    setScalar(_activeShader->uniformLocation(GLSLShaderGenerator::lightingSampler()), lightingTexUnit());
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
  }
  void OpenGLAPI::composite(const RTT & lighting_buffer, const GlobalShaderParams & params, const RTT & dof_buffer, const Depthbuffer& depth_buffer)
  {
    setupShaderDesc(*_compositeShaderDesc, params);
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + lightingTexUnit()));
    lighting_buffer.bind();
    setScalar(_activeShader->uniformLocation(GLSLShaderGenerator::lightingSampler()), lightingTexUnit());
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + dofTexUnit()));
    dof_buffer.bind();
    setScalar(_activeShader->uniformLocation(GLSLShaderGenerator::dofSampler()), dofTexUnit());
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + depthTexUnit()));
    depth_buffer.bind();
    setScalar(_activeShader->uniformLocation(GLSLShaderGenerator::depthSampler()), depthTexUnit());
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
  }
  void OpenGLAPI::endFrame() const
  {
    for (unsigned i = 0; i <= static_cast<unsigned>(heightTexUnit()); i++) {
      _samplerAnisotropic->unbind(i);
    }
  }
  void OpenGLAPI::setAnisotropy(unsigned anisotropy)
  {
    if (_glVersionMajor >= 4 && _glVersionMinor >= 6) {
      float max_ani;
      GL_CHECK(glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_ani));
      _anisotropy = glm::clamp(anisotropy, 1u, static_cast<unsigned>(max_ani));
      _samplerAnisotropic->param(GL_TEXTURE_MAX_ANISOTROPY, static_cast<float>(_anisotropy));
    }
  }
  std::shared_ptr<GLTexture> OpenGLAPI::createTexture(const std::string & path)
  {
    return _textureCache.getOrCreate(path, path);
  }
  std::shared_ptr<OpenGLAPI::MaterialDesc> OpenGLAPI::createMaterial(const std::shared_ptr<Material>& material, const GraphicsSettings& settings)
  {
    return _matDescCache.getOrCreate(material, material, settings);
  }
  std::shared_ptr<GLShaderProgram> OpenGLAPI::createShader(GLShaderSource& vs, GLShaderSource& fs, GLShaderSource& gs)
  {
    std::string key = vs._key + fs._key + gs._key;
    return _shaderCache.getOrCreate(key, vs, fs, gs);
  }
  std::shared_ptr<OpenGLAPI::ShaderDesc> OpenGLAPI::createShaderDesc(const std::shared_ptr<GLShaderProgram>& shader, unsigned flags)
  {
    return _shaderDescCache.getOrCreate(shader, shader, flags);
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
  std::unique_ptr<OpenGLAPI::Depthbuffer> OpenGLAPI::createDepthbuffer(const Vec2u & size, bool stencil)
  {
    auto tex = std::make_unique<GLTexture>(GL_TEXTURE_2D);
    tex->bind();
    tex->param(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    tex->param(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    tex->param(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    tex->param(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    tex->image2D(0, stencil ? GL_DEPTH24_STENCIL8 : GL_DEPTH_COMPONENT24, size, 0, stencil ? GL_DEPTH_STENCIL : GL_DEPTH_COMPONENT, stencil ? GL_UNSIGNED_INT_24_8 : GL_FLOAT, nullptr);
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
  void OpenGLAPI::resizeShadowmap(Shadowmap* shadow_map, const GraphicsSettings& settings)
  {
    shadow_map->image3D(0, GL_DEPTH_COMPONENT24, Vec3u(Vec2u(settings.getShadowMapSize()), static_cast<unsigned>(settings.getFrustumSplits().size())), 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  }
  void OpenGLAPI::recreateShadersAndMaterials(const GraphicsSettings& settings)
  {
    _shaderCache.clear();
    _shaderDescCache.clear();
    for (const auto e : _matDescCache.getElements()) {
      e->create(this, settings);
    }
    createCompositeShader(settings);
  }
  void OpenGLAPI::createBlurShader(const GraphicsSettings & gs)
  {
    GLShaderSource vs, fs;
    _shaderGenerator->createBlurShaderSource(0, gs, vs, fs);
    auto shader = std::make_shared<GLShaderProgram>();
    shader->add(vs);
    shader->add(fs);
    shader->link();
    _sepBlurShaderDesc = std::make_unique<ShaderDesc>(shader, 0);
  }
  void OpenGLAPI::createCompositeShader(const GraphicsSettings & gs)
  {
    unsigned ss_flags = ShaderSetupFlags::NONE;
    if (gs.getDepthOfField()) {
      ss_flags |= ShaderSetupFlags::P_INVERSE;
    }
    GLShaderSource vs, fs;
    _shaderGenerator->createCompositeShaderSource(gs, vs, fs);
    auto shader = std::make_shared<GLShaderProgram>();
    shader->add(vs);
    shader->add(fs);
    shader->link();
    _compositeShaderDesc = std::make_unique<ShaderDesc>(shader, ss_flags);
  }
  std::vector<std::shared_ptr<Material>> OpenGLAPI::getAllMaterials()
  {
    std::vector<std::shared_ptr<Material>> materials;
    for (const auto& e : _matDescCache.getElements()) {
      materials.push_back(e->getMaterial());
    }
    return materials;
  }
  const std::shared_ptr<OpenGLAPI::ShaderDesc>& OpenGLAPI::getSkyboxShaderDesc() const
  {
    return _skydomeShaderDesc;
  }
  void OpenGLAPI::writeShadersToDisk() const
  {
    for (const auto& shader : _shaderCache.getElements()) {
      for (const auto& source : shader->getSources()) {
        std::ofstream os("generated/" + source._key);
        os << source._source;
      }
    }
  }
  void OpenGLAPI::checkFramebufferStatus()
  {
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
      std::cout << "Framebuffer imcomplete" << std::endl;
    }
  }
  void OpenGLAPI::setColorBuffers(const std::vector<RTT*>& rtts)
  {
    _offScreenFramebuffer->bind();
    _offScreenFramebuffer->clearAttachments();
    std::vector<GLenum> draw_buffers;
    if (rtts.size()) {
      unsigned i = 0;
      for (const auto& rtt : rtts) {
        draw_buffers.push_back(GL_COLOR_ATTACHMENT0 + i);
        _offScreenFramebuffer->texture(draw_buffers.back(), rtt, 0);
        i++;
      }
    }
    else {
      draw_buffers.push_back(GL_NONE);
    }
    GL_CHECK(glDrawBuffers(static_cast<GLsizei>(draw_buffers.size()), draw_buffers.data()));
  }
  OpenGLAPI::MeshGeometryStorage::MeshGeometryStorage() :
    _vboAppend(GL_ARRAY_BUFFER),
    _iboAppend(GL_ELEMENT_ARRAY_BUFFER),
    _meshDataCache(SoftwareCache<std::shared_ptr<Mesh>, MeshData, const std::shared_ptr<Mesh>&>([this](
      const std::shared_ptr<Mesh>& mesh) {
    MeshData mesh_data;
    mesh_data._count = static_cast<GLsizei>(mesh->getIndices().size());
    mesh_data._baseVertex = static_cast<GLint>(_baseVertex);
    mesh_data._indices = reinterpret_cast<GLvoid*>(_indices);
    _baseVertex += mesh->getVertices().size();
    _vboAppend.appendData(mesh->getVertices().data(), mesh->getVertices().size());
    if (mesh->getVertices().size() - 1 <= static_cast<size_t>(std::numeric_limits<unsigned short>::max())) {
      std::vector<unsigned short> indices;
      for (const auto& i : mesh->getIndices()) {
        indices.push_back(static_cast<unsigned short>(i));
      }
      _indices += indices.size() * sizeof(indices.front());
      _iboAppend.appendData(indices.data(), indices.size());
      mesh_data._type = GL_UNSIGNED_SHORT;
    }
    else {
      _indices += mesh->getIndices().size() * sizeof(mesh->getIndices().front());
      _iboAppend.appendData(mesh->getIndices().data(), mesh->getIndices().size());
      mesh_data._type = GL_UNSIGNED_INT;
    }
    _vao.bind();
    _vboAppend.getBuffer().bind();
    _iboAppend.getBuffer().bind();
    for (unsigned i = 0; i < 5; i++) {
      GL_CHECK(glEnableVertexAttribArray(i));
    }
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _position))));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _normal))));
    GL_CHECK(glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _uv))));
    GL_CHECK(glVertexAttribPointer(3, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _tangent))));
    GL_CHECK(glVertexAttribPointer(4, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _bitangent))));
    GL_CHECK(glBindVertexArray(0));
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
  OpenGLAPI::MeshGeometryStorage::MeshData OpenGLAPI::MeshGeometryStorage::addMesh(const std::shared_ptr<Mesh>& mesh)
  {
    return _meshDataCache.getOrCreate(mesh, mesh);
  }
  OpenGLAPI::MaterialDesc::MaterialDesc(const std::shared_ptr<Material>& material, OpenGLAPI * api, const GraphicsSettings& settings) : _material(material)
  {
    create(material, api, settings);
  }
  void OpenGLAPI::MaterialDesc::create(OpenGLAPI * api, const GraphicsSettings & settings)
  {
    create(_material, api, settings);
  }
  void OpenGLAPI::MaterialDesc::create(const std::shared_ptr<Material>& material, OpenGLAPI* api, const GraphicsSettings& settings)
  {
    _activeShader = &api->_activeShader;
    _materialSetupFuncs.clear();
    _materialSetupFuncsDepth.clear();
    _diffuseMap = api->createTexture(material->getDiffusePath());
    _normalMap = api->createTexture(material->getNormalPath());
    _alphaMap = api->createTexture(material->getOpacityPath());
    _heightMap = api->createTexture(material->getHeightPath());
    using FLAG = GLSLShaderGenerator::MeshRenderFlag;
    unsigned flag = FLAG::NONE;
    if (_diffuseMap) {
      flag |= FLAG::DIFFUSE_MAP;
      _materialSetupFuncs.push_back(api->_materialSetup->getDiffuseSetup());
    }
    else {
      _materialSetupFuncs.push_back(api->_materialSetup->getDiffuseColorSetup());
    }
    if (_alphaMap) {
      flag |= FLAG::ALPHA_MAP;
      _materialSetupFuncs.push_back(api->_materialSetup->getAlphaSetup());
      _materialSetupFuncsDepth.push_back(api->_materialSetup->getAlphaSetup());
    }
    if (_normalMap && settings.getNormalMapping()) {
      flag |= FLAG::NORMAL_MAP;
      _materialSetupFuncs.push_back(api->_materialSetup->getNormalSetup());
    }
    if (_normalMap && _heightMap && settings.getNormalMapping() && settings.getParallaxMapping()) {
      flag |= FLAG::HEIGHT_MAP;
      _materialSetupFuncs.push_back(api->_materialSetup->getHeightSetup());
      if (settings.getReliefMapping()) {
        _materialSetupFuncs.push_back(api->_materialSetup->getReliefMappingSetup());
      }
    }
    auto fragment_file = api->_shaderGenerator->createMeshFragmentShaderSource(flag, settings);
    auto vertex_file = api->_shaderGenerator->createMeshVertexShaderSource(flag, settings);
    unsigned ss_flags = ShaderSetupFlags::LIGHTING | ShaderSetupFlags::VP;
    if (settings.gammaEnabled()) {
      ss_flags |= ShaderSetupFlags::GAMMA;
    }
    if (settings.getShadows() || settings.getShadowsPCF()) {
      ss_flags |= ShaderSetupFlags::SHADOWS;
    }
    _meshShaderDesc = api->createShaderDesc(api->createShader(vertex_file, fragment_file), ss_flags);
    vertex_file = api->_shaderGenerator->createMeshVertexShaderSource(flag | FLAG::WIND, settings);
    if (settings.getWindAnimations()) {
      ss_flags |= ShaderSetupFlags::WIND | ShaderSetupFlags::TIME;
    }
    _meshShaderDescWind = api->createShaderDesc(api->createShader(vertex_file, fragment_file), ss_flags);

    auto vertex_shadow_file = api->_shaderGenerator->createMeshVertexShaderDepthSource(flag, settings);
    auto vertex_shadow_file_wind = api->_shaderGenerator->createMeshVertexShaderDepthSource(flag | FLAG::WIND, settings);
    auto fragment_shadow_file = api->_shaderGenerator->createMeshFragmentShaderDepthSource(flag, settings);
    auto fragment_shadow_file_wind = api->_shaderGenerator->createMeshFragmentShaderDepthSource(flag | FLAG::WIND, settings);
    _meshShaderDescDepth = api->createShaderDesc(api->createShader(vertex_shadow_file, fragment_shadow_file), ShaderSetupFlags::VP);
    ss_flags = ShaderSetupFlags::VP;
    if (settings.getWindAnimations()) {
      ss_flags |= ShaderSetupFlags::WIND | ShaderSetupFlags::TIME;
    }
    _meshShaderDescWindDepth = api->createShaderDesc(api->createShader(vertex_shadow_file_wind, fragment_shadow_file_wind), ss_flags);
  }
  void OpenGLAPI::MaterialDesc::setup() const
  {
    for (const auto& f : _materialSetupFuncs) {
      f->setup(*_activeShader, *this);
    }
    setScalar((*_activeShader)->uniformLocation(GLSLShaderGenerator::ambientConstant()), _material->getKa());
    setScalar((*_activeShader)->uniformLocation(GLSLShaderGenerator::diffuseConstant()), _material->getKd());
    setScalar((*_activeShader)->uniformLocation(GLSLShaderGenerator::specularConstant()), _material->getKs());
    setScalar((*_activeShader)->uniformLocation(GLSLShaderGenerator::specularExponent()), _material->getSpecularExponent());
  }
  void OpenGLAPI::MaterialDesc::setupDepth() const
  {
    for (const auto& f : _materialSetupFuncsDepth) {
      f->setup(*_activeShader, *this);
    }
  }
  const std::shared_ptr<OpenGLAPI::ShaderDesc>& OpenGLAPI::MaterialDesc::getMeshShaderDesc(bool has_wind) const
  {
    return has_wind ? _meshShaderDescWind : _meshShaderDesc;
  }
  const std::shared_ptr<OpenGLAPI::ShaderDesc>& OpenGLAPI::MaterialDesc::getMeshShaderDescDepth(bool has_wind) const
  {
    return has_wind ? _meshShaderDescWindDepth : _meshShaderDescDepth;
  }
  const std::shared_ptr<Material>& OpenGLAPI::MaterialDesc::getMaterial() const
  {
    return _material;
  }
  const std::shared_ptr<GLTexture>& OpenGLAPI::MaterialDesc::diffuseMap() const
  {
    return _diffuseMap;
  }
  const std::shared_ptr<GLTexture>& OpenGLAPI::MaterialDesc::normalMap() const
  {
    return _normalMap;
  }
  const std::shared_ptr<GLTexture>& OpenGLAPI::MaterialDesc::alphaMap() const
  {
    return _alphaMap;
  }
  const std::shared_ptr<GLTexture>& OpenGLAPI::MaterialDesc::heightMap() const
  {
    return _heightMap;
  }
  OpenGLAPI::ShaderDesc::ShaderDesc(const std::shared_ptr<GLShaderProgram>& shader, unsigned flags) : _shader(shader)
  {
    if (flags & ShaderSetupFlags::VP) {
      _setupFuncs.push_back([this](const GlobalShaderParams& params) {
        setMatrix(_shader->uniformLocation(GLSLShaderGenerator::viewProjectionMatrix()), *params._VP);
      });
    }
    if (flags & ShaderSetupFlags::LIGHTING) {
      _setupFuncs.push_back([this](const GlobalShaderParams& params) {
        setVector(_shader->uniformLocation(GLSLShaderGenerator::lightPositionWorld()), *params._lightPosWorld);
        setVector(_shader->uniformLocation(GLSLShaderGenerator::lightIntensity()), *params._lightIntensity);
        setVector(_shader->uniformLocation(GLSLShaderGenerator::cameraPositionWorld()), params._camPosworld);
      });
    }
    if (flags & ShaderSetupFlags::SHADOWS) {
      _setupFuncs.push_back([this](const GlobalShaderParams& params) {
        setScalar(_shader->uniformLocation(GLSLShaderGenerator::shadowSampler()), shadowTexUnit());
        setMatrixArray(_shader->uniformLocation(GLSLShaderGenerator::worldToLightMatrices()), params._worldToLight.front(), static_cast<unsigned>(params._worldToLight.size()));
        setScalar(_shader->uniformLocation(GLSLShaderGenerator::numfrustumSplits()), static_cast<int>(params._worldToLight.size()));
        setScalarArray(_shader->uniformLocation(GLSLShaderGenerator::frustumSplits()), params._smFrustumSplits->front(), static_cast<unsigned>(params._smFrustumSplits->size()));
        setScalar(_shader->uniformLocation(GLSLShaderGenerator::shadowMapBias()), params._smBias);
        setScalar(_shader->uniformLocation(GLSLShaderGenerator::shadowDarkenFactor()), params._shadowDarkenFactor);
      });
    }
    if (flags & ShaderSetupFlags::TIME) {
      _setupFuncs.push_back([this](const GlobalShaderParams& params) {
        setScalar(_shader->uniformLocation(GLSLShaderGenerator::time()), params._time);
      });
    }
    if (flags & ShaderSetupFlags::WIND) {
      _setupFuncs.push_back([this](const GlobalShaderParams& params) {
        setVector(_shader->uniformLocation(GLSLShaderGenerator::windDir()), params._windParams._dir);
        setVector(_shader->uniformLocation(GLSLShaderGenerator::windMovement()), params._windParams._movement);
        setScalar(_shader->uniformLocation(GLSLShaderGenerator::windFrequency()), params._windParams._frequency);
        setScalar(_shader->uniformLocation(GLSLShaderGenerator::windStrength()), params._windParams._strength);
      });
    }
    if (flags & ShaderSetupFlags::GAMMA) {
      _setupFuncs.push_back([this](const GlobalShaderParams& params) {
        setScalar(_shader->uniformLocation(GLSLShaderGenerator::gamma()), params._gamma);
      });
    }
    if (flags & ShaderSetupFlags::P_INVERSE) {
      _setupFuncs.push_back([this](const GlobalShaderParams& params) {
        setMatrix(_shader->uniformLocation(GLSLShaderGenerator::projectionMatrixInverse()), inverse(params._projectionMatrix));
      });
    }
  }
  void OpenGLAPI::ShaderDesc::setup(const GlobalShaderParams & params) const
  {
    for (const auto& f : _setupFuncs) {
      f(params);
    }
  }
  const std::shared_ptr<GLShaderProgram>& OpenGLAPI::ShaderDesc::getShader() const
  {
    return _shader;
  }
}