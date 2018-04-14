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
    _matDescCache(SoftwareCache<std::shared_ptr<Material>, std::shared_ptr<MaterialDesc>, const std::shared_ptr<Material>&, const Settings&>(
      [this](const std::shared_ptr<Material>& material, const Settings&  settings) {
    return std::make_shared<MaterialDesc>(material, this, settings);
  })),
    _shaderDescCache(SoftwareCache<std::shared_ptr<GLShaderProgram>, std::shared_ptr<ShaderDesc>, const std::shared_ptr<GLShaderProgram>&, bool, bool, bool, bool, bool, bool>(
      [this](const std::shared_ptr<GLShaderProgram>& shader, bool needs_shadows, bool needs_time, bool needs_wind, bool needs_VP, bool needs_lighting, bool needs_lightVP) {
    return std::make_shared<ShaderDesc>(shader, needs_shadows, needs_time, needs_wind, needs_VP, needs_lighting, needs_lightVP);
  }))
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
    _compositeShader = createShader("assets/opengl/vs_screen.glsl", "assets/opengl/fs_composite.glsl");
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
      e->reload();
    }
  }
  void OpenGLAPI::beginFrame() const
  {
    if (_anisotropy > 1) {
      for (unsigned i = 0; i <= 3; i++) {
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
    _activeShader = desc.getShader().get();
    _activeShader->bind();
    desc.setup(params);
  }
  void OpenGLAPI::bindShadowmap(const Shadowmap & shadowmap) const
  {
    GL_CHECK(glActiveTexture(GL_TEXTURE4));
    shadowmap.bind();
  }
  void OpenGLAPI::renderMesh(const MeshGeometryStorage::MeshData& mesh_data, const Mat4f& model_matrix, const Mat3f& model_matrix_inverse) const
  {
    setMatrix(_activeShader->uniformLocation("M"), model_matrix);
    setMatrixTranspose(_activeShader->uniformLocation("M_i"), model_matrix_inverse);
    GL_CHECK(glDrawElementsBaseVertex(GL_TRIANGLES, mesh_data._count, GL_UNSIGNED_INT, mesh_data._indices, mesh_data._baseVertex));
  }
  void OpenGLAPI::renderMesh(const MeshGeometryStorage::MeshData & mesh_data, const Mat4f & model_matrix, const Mat3f & model_matrix_inverse, const WindParamsLocal& params, const AABB& aabb) const
  {
    setMatrix(_activeShader->uniformLocation("M"), model_matrix);
    setMatrixTranspose(_activeShader->uniformLocation("M_i"), model_matrix_inverse);
    setScalar(_activeShader->uniformLocation(GLSLShaderGenerator::windPivot()), params._pivotWorld);
    setScalar(_activeShader->uniformLocation(GLSLShaderGenerator::windExponent()), params._bendFactorExponent);
    setVector(_activeShader->uniformLocation(GLSLShaderGenerator::bbMin()), aabb.getMin());
    setVector(_activeShader->uniformLocation(GLSLShaderGenerator::bbMax()), aabb.getMax());
    GL_CHECK(glDrawElementsBaseVertex(GL_TRIANGLES, mesh_data._count, GL_UNSIGNED_INT, mesh_data._indices, mesh_data._baseVertex));
  }
  void OpenGLAPI::renderMesh(const MeshGeometryStorage::MeshData & mesh_data, const Mat4f & model_matrix, const WindParamsLocal & params, const AABB & aabb) const
  {
    setMatrix(_activeShader->uniformLocation("M"), model_matrix);
    setScalar(_activeShader->uniformLocation(GLSLShaderGenerator::windPivot()), params._pivotWorld);
    setScalar(_activeShader->uniformLocation(GLSLShaderGenerator::windExponent()), params._bendFactorExponent);
    setVector(_activeShader->uniformLocation(GLSLShaderGenerator::bbMin()), aabb.getMin());
    setVector(_activeShader->uniformLocation(GLSLShaderGenerator::bbMax()), aabb.getMax());
    GL_CHECK(glDrawElementsBaseVertex(GL_TRIANGLES, mesh_data._count, GL_UNSIGNED_INT, mesh_data._indices, mesh_data._baseVertex));
  }
  void OpenGLAPI::renderMeshMVP(const MeshGeometryStorage::MeshData & mesh_data, const Mat4f & mvp) const
  {
    setMatrix(_activeShader->uniformLocation("MVP"), mvp);
    GL_CHECK(glDrawElementsBaseVertex(GL_TRIANGLES, mesh_data._count, GL_UNSIGNED_INT, mesh_data._indices, mesh_data._baseVertex));
  }
  void OpenGLAPI::renderAABBs(const std::vector<AABB*>& aabbs, const Mat4f& transform, const Vec3f& col)
  {
    _vaoAABB->bind();
    bindShader(_aabbShader.get());
    setMatrix(_activeShader->uniformLocation("VP"), transform);
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
  void OpenGLAPI::composite(const RTT* lighting_buffer)
  {
    _compositeShader->bind();
    GL_CHECK(glActiveTexture(GL_TEXTURE5));
    lighting_buffer->bind();
    setScalar(_compositeShader->uniformLocation("ts_l"), 5);
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
  }
  void OpenGLAPI::endFrame() const
  {
    for (unsigned i = 0; i <= 3; i++) {
      _samplerAnisotropic->unbind(i);
    }
  }
  void OpenGLAPI::setAnisotropy(unsigned anisotropy)
  {
    if (_glVersionMajor >= 4 && _glVersionMinor >= 6) {
      float max_ani;
      GL_CHECK(glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_ani));
      _anisotropy = glm::clamp(anisotropy, 1u, static_cast<unsigned>(max_ani));
      _samplerAnisotropic->param(GL_TEXTURE_MAX_ANISOTROPY, _anisotropy);
    }
  }
  std::shared_ptr<GLTexture> OpenGLAPI::createTexture(const std::string & path)
  {
    return _textureCache.getOrCreate(path, path);
  }
  std::shared_ptr<OpenGLAPI::MaterialDesc> OpenGLAPI::createMaterial(const std::shared_ptr<Material>& material, const Settings& settings)
  {
    return _matDescCache.getOrCreate(material, material, settings);
  }
  std::shared_ptr<GLShaderProgram> OpenGLAPI::createShader(const std::string & vertex_file, const std::string & fragment_file, const std::string& geometry_file)
  {
    std::string key = vertex_file + fragment_file + geometry_file;
    return _shaderCache.getOrCreate(key, vertex_file, fragment_file, geometry_file);
  }
  std::shared_ptr<OpenGLAPI::ShaderDesc> OpenGLAPI::createShaderDesc(const std::shared_ptr<GLShaderProgram>& shader, bool needs_shadows, 
    bool needs_time, bool needs_wind, bool needs_VP, bool needs_lighting, bool needs_lightVP)
  {
    return _shaderDescCache.getOrCreate(shader, shader, needs_shadows, needs_time, needs_wind, needs_VP, needs_lighting, needs_lightVP);
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
  std::unique_ptr<OpenGLAPI::Shadowmap> OpenGLAPI::createShadowmap(const Vec2u & size, const Settings& settings)
  {
    auto tex = std::make_unique<GLTexture>(GL_TEXTURE_2D_ARRAY);
    tex->bind();
    GLint filter = settings._shadowPercentageCloserFiltering ? GL_LINEAR : GL_NEAREST;
    tex->param(GL_TEXTURE_MIN_FILTER, filter);
    tex->param(GL_TEXTURE_MAG_FILTER, filter);
    tex->param(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    tex->param(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    tex->param(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    tex->param(GL_TEXTURE_BORDER_COLOR, Vec4f(std::numeric_limits<float>::max()).ptr());
    if (settings._shadowPercentageCloserFiltering) {
      tex->param(GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
      tex->param(GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
    }
    tex->image3D(0, GL_DEPTH_COMPONENT24, Vec3u(size, static_cast<unsigned>(settings._smFrustumSplits.size())), 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    return tex;
  }
  void OpenGLAPI::recreateShadersAndMaterials(const Settings& settings)
  {
    _shaderGenerator->regenerateShaders(settings);
    _shaderCache.clear();
    _shaderDescCache.clear();
    for (const auto e : _matDescCache.getElements()) {
      e->create(this, settings);
    }
  }
  std::vector<std::shared_ptr<Material>> OpenGLAPI::getAllMaterials()
  {
    std::vector<std::shared_ptr<Material>> materials;
    for (const auto& e : _matDescCache.getElements()) {
      materials.push_back(e->getMaterial());
    }
    return materials;
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
    _indices += mesh->getIndices().size() * sizeof(mesh->getIndices().front());
    _baseVertex += mesh->getVertices().size();
    _vboAppend->appendData(mesh->getVertices().data(), mesh->getVertices().size());
    _iboAppend->appendData(mesh->getIndices().data(), mesh->getIndices().size());
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
  OpenGLAPI::MaterialDesc::MaterialDesc(const std::shared_ptr<Material>& material, OpenGLAPI * api, const Settings& settings) : _material(material)
  {
    create(material, api, settings);
  }
  void OpenGLAPI::MaterialDesc::create(OpenGLAPI * api, const Settings & settings)
  {
    create(_material, api, settings);
  }
  void OpenGLAPI::MaterialDesc::create(const std::shared_ptr<Material>& material, OpenGLAPI* api, const Settings& settings)
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
      _materialSetupFuncs.push_back([this](GLShaderProgram* shader) {
        GL_CHECK(glActiveTexture(GL_TEXTURE0));
        _diffuseMap->bind();
        setScalar(shader->uniformLocation(GLSLShaderGenerator::diffuseSampler()), 0);
      });
    }
    else {
      _materialSetupFuncs.push_back([this](GLShaderProgram* shader) {
        setVector(shader->uniformLocation("d_col"), _material->getDiffuseColor());
      });
    }
    if (_alphaMap) {
      flag |= FLAG::ALPHA_MAP;
      _materialSetupFuncs.push_back([this](GLShaderProgram* shader) {
        GL_CHECK(glActiveTexture(GL_TEXTURE1));
        _alphaMap->bind();
        setScalar(shader->uniformLocation(GLSLShaderGenerator::alphaSampler()), 1);
      });
      _materialSetupFuncsDepth.push_back(_materialSetupFuncs.back());
    }
    if (_normalMap && settings._normalMapping) {
      flag |= FLAG::NORMAL_MAP;
      _materialSetupFuncs.push_back([this](GLShaderProgram* shader) {
        GL_CHECK(glActiveTexture(GL_TEXTURE2));
        _normalMap->bind();
        setScalar(shader->uniformLocation(GLSLShaderGenerator::normalSampler()), 2);
      });
    }
    if (_normalMap &&_heightMap && (settings._normalMapping && (settings._parallaxMapping || settings._reliefMapping))) {
      flag |= FLAG::PARALLAX_MAP;
      _materialSetupFuncs.push_back([this](GLShaderProgram* shader) {
        GL_CHECK(glActiveTexture(GL_TEXTURE3));
        _heightMap->bind();
        setScalar(shader->uniformLocation(GLSLShaderGenerator::heightSampler()), 3);
        setScalar(shader->uniformLocation(GLSLShaderGenerator::parallaxHeightScale()), _material->getParallaxHeightScale());
      });
      if (settings._reliefMapping) {
        _materialSetupFuncs.push_back([this](GLShaderProgram* shader) {
          setScalar(shader->uniformLocation(GLSLShaderGenerator::parallaxMinSteps()), _material->getParallaxMinSteps());
          setScalar(shader->uniformLocation(GLSLShaderGenerator::parallaxMaxSteps()), _material->getParallaxMaxSteps());
          setScalar(shader->uniformLocation(GLSLShaderGenerator::parallaxBinarySearchSteps()), _material->getParallaxBinarySearchSteps());
        });
      }
    }
  //  _smShader = api->createShader("assets/opengl/vs_shadow.glsl", "assets/opengl/fs_shadow.glsl");
    auto fragment_file = api->_shaderGenerator->createMeshFragmentShaderFile(flag, settings);
    auto vertex_file = api->_shaderGenerator->createMeshVertexShaderFile(flag, settings);
    _meshShaderDesc = api->createShaderDesc(api->createShader(vertex_file, fragment_file), settings._shadows || settings._shadowPercentageCloserFiltering, false, false, true, true, false);
    vertex_file = api->_shaderGenerator->createMeshVertexShaderFile(flag | FLAG::WIND, settings);
    _meshShaderDescWind = api->createShaderDesc(api->createShader(vertex_file, fragment_file), settings._shadows || settings._shadowPercentageCloserFiltering, settings._windAnimations, settings._windAnimations, true, true, false);

    auto vertex_shadow_file = api->_shaderGenerator->createMeshVertexShaderFileDepth(flag, settings);
    auto vertex_shadow_file_wind = api->_shaderGenerator->createMeshVertexShaderFileDepth(flag | FLAG::WIND, settings);
    auto fragment_shadow_file = api->_shaderGenerator->createMeshFragmentShaderFileDepth(flag, settings);
    auto fragment_shadow_file_wind = api->_shaderGenerator->createMeshFragmentShaderFileDepth(flag | FLAG::WIND, settings);
    _meshShaderDescDepth = api->createShaderDesc(api->createShader(vertex_shadow_file, fragment_shadow_file), false, false, false, false, false, false);
    _meshShaderDescWindDepth = api->createShaderDesc(api->createShader(vertex_shadow_file_wind, fragment_shadow_file_wind), false, settings._windAnimations, settings._windAnimations, true, false, true);
  }
  void OpenGLAPI::MaterialDesc::setup(GLShaderProgram* shader) const
  {
    for (const auto& f : _materialSetupFuncs) {
      f(shader);
    }
    setScalar(shader->uniformLocation("ka"), _material->getKa());
    setScalar(shader->uniformLocation("kd"), _material->getKd());
    setScalar(shader->uniformLocation("ks"), _material->getKs());
    setScalar(shader->uniformLocation("s_e"), _material->getSpecularExponent());
  }
  void OpenGLAPI::MaterialDesc::setupDepth(GLShaderProgram * shader) const
  {
    for (const auto& f : _materialSetupFuncsDepth) {
      f(shader);
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
 /* const std::shared_ptr<OpenGLAPI::MaterialDesc::ShaderProgram>& OpenGLAPI::MaterialDesc::getSMShader() const
  {
    return _smShader;
  }*/
  const std::shared_ptr<Material>& OpenGLAPI::MaterialDesc::getMaterial() const
  {
    return _material;
  }
  OpenGLAPI::ShaderDesc::ShaderDesc(const std::shared_ptr<GLShaderProgram>& shader, bool needs_shadows,
    bool needs_time, bool needs_wind, bool needs_VP, bool needs_lighting, bool needs_lightVP) : _shader(shader)
  {
    if (needs_VP) {
      _setupFuncs.push_back([this](const GlobalShaderParams& params) {
        setMatrix(_shader->uniformLocation("VP"), params._VP);
      });
    }
    if (needs_lighting) {
      _setupFuncs.push_back([this](const GlobalShaderParams& params) {
        setVector(_shader->uniformLocation("lpos_ws"), params._lightPosWorld);
        setVector(_shader->uniformLocation("I_in"), params._lightIntensity);
        setVector(_shader->uniformLocation("cp_ws"), params._camPosworld);
      });
    }
    if (needs_shadows) {
      _setupFuncs.push_back([this](const GlobalShaderParams& params) {
        setScalar(_shader->uniformLocation("ts_sm"), 4);
        setMatrixArray(_shader->uniformLocation("w_to_l"), params._worldToLight.front(), static_cast<unsigned>(params._worldToLight.size()));
        setScalar(_shader->uniformLocation("nfs"), static_cast<int>(params._worldToLight.size()));
        setScalarArray(_shader->uniformLocation("fs"), params._smFrustumSplits.front(), static_cast<unsigned>(params._smFrustumSplits.size()));
        setScalar(_shader->uniformLocation(GLSLShaderGenerator::shadowMapBias()), params._smBias);
      });
    }
    if (needs_lightVP) {
      _setupFuncs.push_back([this](const GlobalShaderParams& params) {
        setMatrix(_shader->uniformLocation("VP"), *params._lightVP);
      });
    }
    if (needs_time) {
      _setupFuncs.push_back([this](const GlobalShaderParams& params) {
        setScalar(_shader->uniformLocation(GLSLShaderGenerator::time()), params._time);
      });
    }
    if (needs_wind) {
      _setupFuncs.push_back([this](const GlobalShaderParams& params) {
        setVector(_shader->uniformLocation(GLSLShaderGenerator::windDir()), params._windParams._dir);
        setVector(_shader->uniformLocation(GLSLShaderGenerator::windMovement()), params._windParams._movement);
        setScalar(_shader->uniformLocation(GLSLShaderGenerator::windFrequency()), params._windParams._frequency);
        setScalar(_shader->uniformLocation(GLSLShaderGenerator::windStrength()), params._windParams._strength);
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