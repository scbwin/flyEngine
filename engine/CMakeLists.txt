cmake_minimum_required(VERSION 3.0)
project (flyEngine)

file (GLOB SOURCE_FILES
	"source/*.cpp"
	"source/physics/*.cpp"
	"source/dx11/*.cpp"
	"source/opengl/*.cpp"
)

file (GLOB HEADER_FILES
	"include/*.h"
	"include/physics/*.h"
	"include/dx11/*.h"
	"include/opengl/*.h"
	"include/math/*.h"
)

#file (GLOB_RECURSE BULLET_SOURCES 
#	"C:/bullet3/src/*.cpp"
#)

set(GLM_DIR "" CACHE PATH "")
set(SOIL_DIR "" CACHE PATH "")
set (SOIL_LIB "" CACHE FILEPATH "")
set (DXUT_DIR "" CACHE PATH "")
set (DXUT_LIB_DEBUG "" CACHE FILEPATH "")
set (DXUT_LIB_RELEASE "" CACHE FILEPATH "")
set (FX_DIR "" CACHE PATH "")
set (FX_LIB_DEBUG "" CACHE FILEPATH "")
set (FX_LIB_RELEASE "" CACHE FILEPATH "")
set (DXTEX_DIR "" CACHE PATH "")
set (DXTEX_LIB_DEBUG "" CACHE FILEPATH "")
set (DXTEX_LIB_RELEASE "" CACHE FILEPATH "")
set (DXTK_DIR "" CACHE PATH "")
set (DXTK_LIB_DEBUG "" CACHE FILEPATH "")
set (DXTK_LIB_RELEASE "" CACHE FILEPATH "")

IF (WIN32)
	find_package(GLEW REQUIRED)
ENDIF()
find_package(OpenGL)
find_package(assimp REQUIRED)
find_package(OpenCV REQUIRED)

set(INCLUDE_DIRS 
	include
)

include_directories(${INCLUDE_DIRS}  ${SOIL_DIR} ${GLM_DIR} ${GLEW_INCLUDE_DIRS} ${ASSIMP_INCLUDE_DIRS} ${OpenCV_DIRS} ${DXUT_DIR} ${FX_DIR} ${DXTEX_DIR} ${DXTK_DIR})
add_library(flyEngine STATIC ${HEADER_FILES} ${SOURCE_FILES} ${BULLET_SOURCES})

target_link_libraries(flyEngine ${SOIL_LIB} ${GLEW_LIBRARIES} ${GLFW_LIB} ${OPENGL_gl_LIBRARY} ${ASSIMP_LIBRARY_DIRS}/${ASSIMP_LIBRARIES}.lib ${OpenCV_LIBRARIES})

target_link_libraries(flyEngine 
	debug ${FLY_LIB_DEBUG} optimized ${FLY_LIB_RELEASE}
	debug ${DXUT_LIB_DEBUG} optimized ${DXUT_LIB_RELEASE}
	debug ${FX_LIB_DEBUG} optimized ${FX_LIB_RELEASE}
	debug ${DXTEX_LIB_DEBUG} optimized ${DXTEX_LIB_RELEASE}
	debug ${DXTK_LIB_DEBUG} optimized ${DXTK_LIB_RELEASE}
)