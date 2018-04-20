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

namespace fly
{
  OpenGLAPI::OpenGLAPI() :
    _shaderGenerator(std::make_unique<GLSLShaderGenerator>()),
    _shaderCache(SoftwareCache<std::string, std::shared_ptr<GLShaderProgram>, const std::string&,
      const std::string&, const std::string& >([](const std::string& vs, const std::string& fs, const std::string& gs) {
    auto ret = std::make_shared<GLShaderProgram>();
    ret->create();
    ret->addShaderFromFile(vs, GLShaderProgram::ShaderType::VERTEX);
    if (gs != "") { ret->addShaderFromFile(gs, GLShaderProgram::ShaderType::GEOMETRY); }
    ret->addShaderFromFile(fs, GLShaderProgram::ShaderType::FRAGMENT);
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
    _aabbShader = createShader("assets/opengl/vs_aabb.glsl", "assets/opengl/fs_aabb.glsl", "assets/opengl/gs_aabb.glsl");
    _samplerAnisotropic = std::make_unique<GLSampler>();
    _skydomeShader = createShader("assets/opengl/vs_skybox.glsl", "assets/opengl/fs_skydome_new.glsl");
    _skydomeShaderDesc = createShaderDesc(_skydomeShader, ShaderSetupFlags::VP);
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
      e->reload();
    }
    _skydomeShader->reload();
  }
  void OpenGLAPI::beginFrame() const
  {
    if (_anisotropy > 1) {
      for (unsigned i = 0; i <= heightTexUnit(); i++) {
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
  void OpenGLAPI::renderAABBs(const std::vector<AABB*>& aabbs, const Mat4f& transform, const Vec3f& col)
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
    _offScreenFramebuffer->texture(GL_DEPTH_ATTACHMENT, depth_buffer, 0);
    checkFramebufferStatus();
  }
  void OpenGLAPI::setRendertargets(const std::vector<RTT*>& rtts, const Depthbuffer* depth_buffer, unsigned depth_buffer_layer)
  {
    setColorBuffers(rtts);
    _offScreenFramebuffer->textureLayer(GL_DEPTH_ATTACHMENT, depth_buffer, 0, depth_buffer_layer);
    checkFramebufferStatus();
  }
  void OpenGLAPI::bindBackbuffer(unsigned id) const
  {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, id));
  }
  void OpenGLAPI::composite(const RTT* lighting_buffer, const GlobalShaderParams& params)
  {
    setupShaderDesc(*_compositeShaderDesc, params);
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + lightingTexUnit()));
    lighting_buffer->bind();
    setScalar(_activeShader->uniformLocation(GLSLShaderGenerator::lightingSampler()), lightingTexUnit());
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
  }
  void OpenGLAPI::endFrame() const
  {
    for (unsigned i = 0; i <= heightTexUnit(); i++) {
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
  std::shared_ptr<GLShaderProgram> OpenGLAPI::createShader(const std::string & vertex_file, const std::string & fragment_file, const std::string& geometry_file)
  {
    std::string key = vertex_file + fragment_file + geometry_file;
    return _shaderCache.getOrCreate(key, vertex_file, fragment_file, geometry_file);
  }
  std::shared_ptr<OpenGLAPI::ShaderDesc> OpenGLAPI::createShaderDesc(const std::shared_ptr<GLShaderProgram>& shader, unsigned flags)
  {
    return _shaderDescCache.getOrCreate(shader, shader, flags);
  }
  std::unique_ptr<OpenGLAPI::RTT> OpenGLAPI::createRenderToTexture(const Vec2u & size)
  {
    auto tex = std::make_unique<GLTexture>(GL_TEXTURE_2D);
    tex->bind();
    tex->param(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    tex->param(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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
  std::unique_ptr<OpenGLAPI::Shadowmap> OpenGLAPI::createShadowmap(const Vec2u & size, const GraphicsSettings& settings)
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
    tex->image3D(0, GL_DEPTH_COMPONENT24, Vec3u(size, static_cast<unsigned>(settings.getFrustumSplits().size())), 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    return tex;
  }
  void OpenGLAPI::recreateShadersAndMaterials(const GraphicsSettings& settings)
  {
    _shaderGenerator->regenerateShaders(settings);
    _shaderCache.clear();
    _shaderDescCache.clear();
    for (const auto e : _matDescCache.getElements()) {
      e->create(this, settings);
    }
    createCompositeShaderFile(settings);
  }
  void OpenGLAPI::createCompositeShaderFile(const GraphicsSettings & gs)
  {
    unsigned flags = GLSLShaderGenerator::CompositeFlag::CP_NONE;
    unsigned ss_flags = ShaderSetupFlags::NONE;
    if (gs.exposureEnabled()) {
      flags |= GLSLShaderGenerator::CompositeFlag::EXPOSURE;
      ss_flags |= ShaderSetupFlags::EXPOSURE;
    }
    std::string vs_c, fs_c;
    _shaderGenerator->createCompositeShaderFiles(flags, gs, vs_c, fs_c);
    _compositeShaderDesc = createShaderDesc(createShader(vs_c, fs_c), ss_flags);
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
    _vboAppend(std::make_unique<GLAppendBuffer>(GL_ARRAY_BUFFER)),
    _iboAppend(std::make_unique<GLAppendBuffer>(GL_ELEMENT_ARRAY_BUFFER)),
    _meshDataCache(SoftwareCache<std::shared_ptr<Mesh>, MeshData, const std::shared_ptr<Mesh>&>([this](
      const std::shared_ptr<Mesh>& mesh) {
    MeshData mesh_data;
    mesh_data._count = static_cast<GLsizei>(mesh->getIndices().size());
    mesh_data._baseVertex = static_cast<GLint>(_baseVertex);
    mesh_data._indices = reinterpret_cast<GLvoid*>(_indices);
    _baseVertex += mesh->getVertices().size();
    _vboAppend->appendData(mesh->getVertices().data(), mesh->getVertices().size());
    if (mesh->getVertices().size() - 1 <= static_cast<size_t>(std::numeric_limits<unsigned short>::max())) {
      std::vector<unsigned short> indices;
      for (const auto& i : mesh->getIndices()) {
        indices.push_back(static_cast<unsigned short>(i));
      }
      _indices += indices.size() * sizeof(indices.front());
      _iboAppend->appendData(indices.data(), indices.size());
      mesh_data._type = GL_UNSIGNED_SHORT;
    }
    else {
      _indices += mesh->getIndices().size() * sizeof(mesh->getIndices().front());
      _iboAppend->appendData(mesh->getIndices().data(), mesh->getIndices().size());
      mesh_data._type = GL_UNSIGNED_INT;
    }
    _vao = std::make_unique<GLVertexArray>();
    _vao->bind();
    _vboAppend->getBuffer()->bind();
    _iboAppend->getBuffer()->bind();
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
    _vao->bind();
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
    if (_normalMap &&_heightMap && settings.getNormalMapping() && (settings.getParallaxMapping() || settings.getReliefMapping())) {
      flag |= FLAG::PARALLAX_MAP;
      _materialSetupFuncs.push_back(api->_materialSetup->getHeightSetup());
      if (settings.getReliefMapping()) {
        _materialSetupFuncs.push_back(api->_materialSetup->getReliefMappingSetup());
      }
    }
    auto fragment_file = api->_shaderGenerator->createMeshFragmentShaderFile(flag, settings);
    auto vertex_file = api->_shaderGenerator->createMeshVertexShaderFile(flag, settings);
    unsigned ss_flags = ShaderSetupFlags::LIGHTING | ShaderSetupFlags::VP;;
    if (settings.getShadows() || settings.getShadowsPCF()) {
      ss_flags |= ShaderSetupFlags::SHADOWS;
    }
    _meshShaderDesc = api->createShaderDesc(api->createShader(vertex_file, fragment_file), ss_flags);
    vertex_file = api->_shaderGenerator->createMeshVertexShaderFile(flag | FLAG::WIND, settings);
    if (settings.getWindAnimations()) {
      ss_flags |= ShaderSetupFlags::WIND | ShaderSetupFlags::TIME;
    }
    _meshShaderDescWind = api->createShaderDesc(api->createShader(vertex_file, fragment_file), ss_flags);

    auto vertex_shadow_file = api->_shaderGenerator->createMeshVertexShaderFileDepth(flag, settings);
    auto vertex_shadow_file_wind = api->_shaderGenerator->createMeshVertexShaderFileDepth(flag | FLAG::WIND, settings);
    auto fragment_shadow_file = api->_shaderGenerator->createMeshFragmentShaderFileDepth(flag, settings);
    auto fragment_shadow_file_wind = api->_shaderGenerator->createMeshFragmentShaderFileDepth(flag | FLAG::WIND, settings);
    _meshShaderDescDepth = api->createShaderDesc(api->createShader(vertex_shadow_file, fragment_shadow_file), ShaderSetupFlags::VP);
    ss_flags = ShaderSetupFlags::VP;
    if (settings.getWindAnimations()) {
      ss_flags |= ShaderSetupFlags::WIND | ShaderSetupFlags::TIME;
    }
    _meshShaderDescWindDepth = api->createShaderDesc(api->createShader(vertex_shadow_file_wind, fragment_shadow_file_wind), ss_flags);
  }
  void OpenGLAPI::MaterialDesc::setup(GLShaderProgram* shader) const
  {
    for (const auto& f : _materialSetupFuncs) {
      f->setup(shader, *this);
    }
    setScalar(shader->uniformLocation(GLSLShaderGenerator::ambientConstant()), _material->getKa());
    setScalar(shader->uniformLocation(GLSLShaderGenerator::diffuseConstant()), _material->getKd());
    setScalar(shader->uniformLocation(GLSLShaderGenerator::specularConstant()), _material->getKs());
    setScalar(shader->uniformLocation(GLSLShaderGenerator::specularExponent()), _material->getSpecularExponent());
  }
  void OpenGLAPI::MaterialDesc::setupDepth(GLShaderProgram * shader) const
  {
    for (const auto& f : _materialSetupFuncsDepth) {
      f->setup(shader, *this);
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
        setVector(_shader->uniformLocation(GLSLShaderGenerator::lightPositionWorld()), params._lightPosWorld);
        setVector(_shader->uniformLocation(GLSLShaderGenerator::lightIntensity()), params._lightIntensity);
        setVector(_shader->uniformLocation(GLSLShaderGenerator::cameraPositionWorld()), params._camPosworld);
      });
    }
    if (flags & ShaderSetupFlags::SHADOWS) {
      _setupFuncs.push_back([this](const GlobalShaderParams& params) {
        setScalar(_shader->uniformLocation(GLSLShaderGenerator::shadowSampler()), shadowTexUnit());
        setMatrixArray(_shader->uniformLocation(GLSLShaderGenerator::worldToLightMatrices()), params._worldToLight.front(), static_cast<unsigned>(params._worldToLight.size()));
        setScalar(_shader->uniformLocation(GLSLShaderGenerator::numfrustumSplits()), static_cast<int>(params._worldToLight.size()));
        setScalarArray(_shader->uniformLocation(GLSLShaderGenerator::frustumSplits()), params._smFrustumSplits.front(), static_cast<unsigned>(params._smFrustumSplits.size()));
        setScalar(_shader->uniformLocation(GLSLShaderGenerator::shadowMapBias()), params._smBias);
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
    if (flags & ShaderSetupFlags::EXPOSURE) {
      _setupFuncs.push_back([this](const GlobalShaderParams& params) {
        setScalar(_shader->uniformLocation(GLSLShaderGenerator::exposure()), params._exposure);
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