cmake_minimum_required(VERSION 3.0)
project (flyEngine)

set (IDIR include)
set (SDIR source)

set (HEADER_FILES
	${IDIR}/AABB.h ${IDIR}/Animation.h ${IDIR}/AnimationSystem.h ${IDIR}/AssimpImporter.h ${IDIR}/Billboard.h ${IDIR}/Camera.h ${IDIR}/Component.h ${IDIR}/Engine.h ${IDIR}/Entity.h ${IDIR}/EntityManager.h ${IDIR}/FixedTimestepSystem.h
	${IDIR}/GameTimer.h ${IDIR}/GeometryGenerator.h ${IDIR}/IImporter.h ${IDIR}/Light.h ${IDIR}/Material.h ${IDIR}/Mesh.h ${IDIR}/Model.h ${IDIR}/NoiseGen.h ${IDIR}/Renderables.h ${IDIR}/RenderingSystem.h
	${IDIR}/System.h ${IDIR}/Terrain.h ${IDIR}/TerrainNew.h ${IDIR}/Transform.h ${IDIR}/Vertex.h
	${IDIR}/dx11/debug.h ${IDIR}/dx11/DX11States.h ${IDIR}/dx11/DXUtils.h ${IDIR}/dx11/Effects.h ${IDIR}/dx11/RenderingSystemDX11.h
	${IDIR}/math/FlyMath.h ${IDIR}/math/FlyMatrix.h ${IDIR}/math/FlyVector.h ${IDIR}/math/Helpers.h ${IDIR}/math/Meta.h
	${IDIR}/opengl/GLWrappers.h ${IDIR}/opengl/OpenGLUtils.h ${IDIR}/opengl/RenderingSystemOpenGL.h
	${IDIR}/physics/ParticleSystem.h ${IDIR}/physics/PhysicsSystem.h
)

set (SOURCE_FILES
	${SDIR}/AABB.cpp ${SDIR}/Animation.cpp ${SDIR}/AnimationSystem.cpp ${SDIR}/AssimpImporter.cpp ${SDIR}/Billboard.cpp ${SDIR}/Camera.cpp ${SDIR}/Component.cpp ${SDIR}/Engine.cpp ${SDIR}/Entity.cpp ${SDIR}/EntityManager.cpp ${SDIR}/FixedTimestepSystem.cpp
	${SDIR}/GameTimer.cpp ${SDIR}/GeometryGenerator.cpp ${SDIR}/Light.cpp ${SDIR}/Material.cpp ${SDIR}/Mesh.cpp ${SDIR}/Model.cpp ${SDIR}/NoiseGen.cpp ${SDIR}/Renderables.cpp ${SDIR}/RenderingSystem.cpp
	${SDIR}/System.cpp ${SDIR}/Terrain.cpp ${SDIR}/TerrainNew.cpp ${SDIR}/Transform.cpp 
	${SDIR}/dx11/DX11States.cpp ${SDIR}/dx11/Effects.cpp ${SDIR}/dx11/RenderingSystemDX11.cpp
	${SDIR}/opengl/GLWrappers.cpp ${SDIR}/opengl/OpenGLUtils.cpp ${SDIR}/opengl/RenderingSystemOpenGL.cpp
	${SDIR}/physics/ParticleSystem.cpp ${SDIR}/physics/PhysicsSystem.cpp
)

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

IF (WIN32)
	find_package(GLEW REQUIRED)
ENDIF()
find_package(OpenGL)
find_package(assimp REQUIRED)
find_package(OpenCV REQUIRED)

set(INCLUDE_DIRS 
${IDIR} 
${SOIL_DIR} 
${GLM_DIR} 
${GLEW_INCLUDE_DIRS} 
${ASSIMP_INCLUDE_DIRS} 
${OpenCV_DIRS} 
${DXUT_DIR} 
${FX11_DIR} 
${DXTEX_DIR} 
${DXTK_DIR}
)

set(LIBRARIES
${SOIL_LIB} 
${GLEW_LIBRARIES} 
${GLFW_LIB} 
${OPENGL_gl_LIBRARY} 
${ASSIMP_LIBRARY_DIRS}/${ASSIMP_LIBRARIES}.lib 
${OpenCV_LIBRARIES} 
${DXUT_LIB} 
${FX11_LIB} 
${DXTEX_LIB} 
${DXTK_LIB}
d3d11.lib 
comctl32.lib
)

include_directories(${INCLUDE_DIRS} )
add_library(flyEngine STATIC ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(flyEngine ${LIBRARIES})

install(TARGETS flyEngine 
		ARCHIVE DESTINATION lib
		)
install(DIRECTORY ${IDIR} DESTINATION ".") # Copy headers to install path
#configure_file(flyEngineConfig.cmake ${CMAKE_INSTALL_PREFIX}/flyEngineConfig.cmake)
configure_file(flyEngineConfig.cmake.in ${CMAKE_BINARY_DIR}/flyEngineConfig.cmake)
install(FILES ${CMAKE_BINARY_DIR}/flyEngineConfig.cmake DESTINATION ".")