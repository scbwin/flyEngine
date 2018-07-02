cmake_minimum_required(VERSION 3.0)
project (flyEngine)

set (IDIR include)
set (SDIR source)

set(BUILD_DX11 false CACHE BOOL "")
set(BUILD_PHYSICS false CACHE BOOL "")
set(INCLUDE_OPENCV false CACHE BOOL "")
set(FAST_FLOAT true CACHE BOOL "")

set (HEADER_FILES
	${IDIR}/AABB.h ${IDIR}/Animation.h ${IDIR}/AnimationSystem.h ${IDIR}/AssimpImporter.h ${IDIR}/Billboard.h ${IDIR}/Camera.h ${IDIR}/Engine.h ${IDIR}/FixedTimestepSystem.h
	${IDIR}/GameTimer.h ${IDIR}/GeometryGenerator.h ${IDIR}/IImporter.h ${IDIR}/Light.h ${IDIR}/Material.h ${IDIR}/Mesh.h ${IDIR}/Model.h ${IDIR}/NoiseGen.h ${IDIR}/Renderables.h ${IDIR}/RenderingSystem.h
	${IDIR}/System.h ${IDIR}/Terrain.h ${IDIR}/TerrainNew.h ${IDIR}/Transform.h ${IDIR}/Vertex.h
	${IDIR}/Leakcheck.h
	${IDIR}/math/FlyMath.h ${IDIR}/math/FlyMatrix.h ${IDIR}/math/FlyVector.h ${IDIR}/math/MatVecHelpers.h ${IDIR}/math/Meta.h ${IDIR}/math/MathHelpers.h
	${IDIR}/opengl/GLVertexArray.h ${IDIR}/opengl/GLBuffer.h ${IDIR}/opengl/GLTexture.h ${IDIR}/opengl/GLByteBufferStack.h
	${IDIR}/opengl/OpenGLUtils.h ${IDIR}/opengl/RenderingSystemOpenGL.h ${IDIR}/opengl/OpenGLAPI.h ${IDIR}/renderer/ProjectionParams.h
	${IDIR}/physics/ParticleSystem.h ${IDIR}/physics/PhysicsSystem.h ${IDIR}/Quadtree.h ${IDIR}/Octree.h ${IDIR}/Settings.h ${IDIR}/GraphicsSettings.h ${IDIR}/LevelOfDetail.h ${IDIR}/Timing.h ${IDIR}/renderer/Renderer.h
	${IDIR}/CameraController.h ${IDIR}/PhysicsCameraController.h ${IDIR}/opengl/GLShaderInterface.h ${IDIR}/opengl/GLFramebuffer.h
	${IDIR}/opengl/GLSLShaderGenerator.h ${IDIR}/SoftwareCache.h ${IDIR}/opengl/GLSampler.h ${IDIR}/WindParams.h ${IDIR}/WindParamsLocal.h
    ${IDIR}/opengl/GLMaterialSetup.h ${IDIR}/opengl/GLShaderSetup.h ${IDIR}/opengl/GLShaderProgram.h ${IDIR}/opengl/GLShaderSource.h ${IDIR}/StackPOD.h
	${IDIR}/Flags.h ${IDIR}/MaterialDesc.h ${IDIR}/renderer/MeshRenderables.h ${IDIR}/ShaderDesc.h ${IDIR}/CamSpeedSystem.h
	${IDIR}/GlobalShaderParams.h ${IDIR}/ZNearMapping.h ${IDIR}/Sphere.h ${IDIR}/KdTree.h ${IDIR}/KdTreeOld.h ${IDIR}/Cube.h ${IDIR}/IntersectionTests.h ${IDIR}/CullResult.h
  ${IDIR}/RenderList.h
)

if(${BUILD_PHYSICS})
	set(HEADER_FILES ${HEADER_FILES} ${IDIR}/physics/Bullet3PhysicsSystem.h ${IDIR}/physics/RigidBody.h )
endif()
if(${BUILD_DX11})
	set(HEADER_FILES ${HEADER_FILES} ${IDIR}/dx11/DX11States.h ${IDIR}/dx11/DXUtils.h ${IDIR}/dx11/Effects.h ${IDIR}/dx11/RenderingSystemDX11.h)
endif()

set (SOURCE_FILES
	${SDIR}/AABB.cpp ${SDIR}/Animation.cpp ${SDIR}/AnimationSystem.cpp ${SDIR}/AssimpImporter.cpp ${SDIR}/Billboard.cpp ${SDIR}/Camera.cpp ${SDIR}/Engine.cpp ${SDIR}/FixedTimestepSystem.cpp
	${SDIR}/GameTimer.cpp ${SDIR}/GeometryGenerator.cpp ${SDIR}/Light.cpp ${SDIR}/Material.cpp ${SDIR}/Mesh.cpp ${SDIR}/Model.cpp ${SDIR}/NoiseGen.cpp ${SDIR}/Renderables.cpp ${SDIR}/RenderingSystem.cpp
	${SDIR}/System.cpp ${SDIR}/Terrain.cpp ${SDIR}/TerrainNew.cpp ${SDIR}/Transform.cpp 
	${SDIR}/opengl/OpenGLUtils.cpp ${SDIR}/opengl/RenderingSystemOpenGL.cpp ${SDIR}/opengl/GLTexture.cpp
	${SDIR}/physics/ParticleSystem.cpp ${SDIR}/physics/PhysicsSystem.cpp ${SDIR}/LevelOfDetail.cpp ${SDIR}/Timing.cpp ${SDIR}/opengl/OpenGLAPI.cpp
	${SDIR}/opengl/GLVertexArray.cpp ${SDIR}/opengl/GLBuffer.cpp ${SDIR}/CameraController.cpp ${SDIR}/PhysicsCameraController.cpp ${SDIR}/opengl/GLFramebuffer.cpp
	${SDIR}/opengl/GLSLShaderGenerator.cpp ${SDIR}/opengl/GLSampler.cpp ${SDIR}/GraphicsSettings.cpp
	${SDIR}/opengl/GLMaterialSetup.cpp ${SDIR}/opengl/GLShaderSetup.cpp ${SDIR}/opengl/GLShaderProgram.cpp ${SDIR}/opengl/GLShaderSource.cpp
	${SDIR}/Sphere.cpp ${SDIR}/Cube.cpp
)

if(${BUILD_PHYSICS})
	set(SOURCE_FILES ${SOURCE_FILES} ${SDIR}/physics/Bullet3PhysicsSystem.cpp ${SDIR}/physics/RigidBody.cpp)
endif()
if(${BUILD_DX11})
	set(SOURCE_FILES ${SOURCE_FILES} ${SDIR}/dx11/DX11States.cpp ${SDIR}/dx11/Effects.cpp ${SDIR}/dx11/RenderingSystemDX11.cpp)
endif()

set(GLM_DIR "" CACHE PATH "")
set(SOIL_DIR "" CACHE PATH "")
set (SOIL_LIB "" CACHE FILEPATH "")
set (DXUT_DIR "" CACHE PATH "")
set (DXUT_LIB "" CACHE FILEPATH "")
set (FX11_DIR "" CACHE PATH "")
set (FX11_LIB "" CACHE FILEPATH "")
set (DXTEX_DIR "" CACHE PATH "")
set (DXTEX_LIB "" CACHE FILEPATH "")
set (DXTK_DIR "" CACHE PATH "")
set (DXTK_LIB "" CACHE FILEPATH "")
set (BOOST_DIR "" CACHE PATH "")

IF (WIN32)
	find_package(GLEW REQUIRED)
ENDIF()
find_package(OpenGL)
find_package(assimp REQUIRED)
if(${INCLUDE_OPENCV})
find_package(OpenCV REQUIRED)
endif()
if(${BUILD_PHYSICS})
find_package(Bullet REQUIRED)
endif()

set(INCLUDE_DIRS 
${IDIR} 
${SOIL_DIR} 
${GLM_DIR} 
${GLEW_INCLUDE_DIRS} 
${ASSIMP_INCLUDE_DIRS} 
${OpenCV_DIRS}
${BOOST_DIR}
)

if(${BUILD_PHYSICS})
	set(INCLUDE_DIRS ${INCLUDE_DIRS} ${BULLET_INCLUDE_DIRS})
endif()
if(${BUILD_DX11})
	set(INCLUDE_DIRS ${INCLUDE_DIRS} ${DXUT_DIR} ${FX11_DIR} ${DXTEX_DIR} ${DXTK_DIR})
endif()

set(LIBRARIES
${SOIL_LIB} 
${GLEW_LIBRARIES} 
${GLFW_LIB} 
${OPENGL_gl_LIBRARY} 
${ASSIMP_LIBRARY_DIRS}/${ASSIMP_LIBRARIES}.lib 
${OpenCV_LIBRARIES}
)

if(${BUILD_PHYSICS})
	set(LIBRARIES ${LIBRARIES} ${BULLET_LIBRARIES})
endif()
if(${BUILD_DX11})
	set(LIBRARIES ${LIBRARIES} ${DXUT_LIB} ${FX11_LIB} ${DXTEX_LIB} ${DXTK_LIB} d3d11.lib comctl32.lib)
endif()

if(${INCLUDE_OPENCV})
add_definitions(-DINCLUDE_OPENCV)
endif()

include_directories(${INCLUDE_DIRS} )

if (${FAST_FLOAT})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

add_library(flyEngine STATIC ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(flyEngine ${LIBRARIES})

install(TARGETS flyEngine 
		ARCHIVE DESTINATION lib
		)
install(DIRECTORY ${IDIR} DESTINATION ".") # Copy headers to install path
#configure_file(flyEngineConfig.cmake ${CMAKE_INSTALL_PREFIX}/flyEngineConfig.cmake)
configure_file(flyEngineConfig.cmake.in ${CMAKE_BINARY_DIR}/flyEngineConfig.cmake)
install(FILES ${CMAKE_BINARY_DIR}/flyEngineConfig.cmake DESTINATION ".")