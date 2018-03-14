cmake_minimum_required(VERSION 3.0)
project (dx11_example)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

file (GLOB SOURCE_FILES
	"source/*.cpp"
)

file (GLOB HEADER_FILES
	"../engine/include/*.h"
	"include/*.h"
	"include/physics/*.h"
)

set(GLM_DIR "" CACHE PATH "")
find_package(assimp REQUIRED)
find_package(OpenCV REQUIRED)
set (FLY_LIB_DEBUG "" CACHE FILEPATH "")
set (FLY_LIB_RELEASE "" CACHE FILEPATH "")
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
set (ANT_DIR "" CACHE PATH "")
set (ANT_LIB "" CACHE FILEPATH "")

include_directories(../engine/include/ include/ ${GLM_DIR} ${ASSIMP_INCLUDE_DIRS} ${OpenCV_DIRS} ${DXUT_DIR} ${FX_DIR} ${DXTEX_DIR} ${DXTK_DIR} ${ANT_DIR})
add_executable(dx11_example ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(dx11_example ${ASSIMP_LIBRARY_DIRS}/${ASSIMP_LIBRARIES}.lib  ${OpenCV_LIBRARIES} d3d11.lib comctl32.lib ${ANT_LIB})
target_link_libraries(dx11_example 
	debug ${FLY_LIB_DEBUG} optimized ${FLY_LIB_RELEASE}
	debug ${DXUT_LIB_DEBUG} optimized ${DXUT_LIB_RELEASE}
	debug ${FX_LIB_DEBUG} optimized ${FX_LIB_RELEASE}
	debug ${DXTEX_LIB_DEBUG} optimized ${DXTEX_LIB_RELEASE}
	debug ${DXTK_LIB_DEBUG} optimized ${DXTK_LIB_RELEASE}
)

if (WIN32)
set_target_properties(dx11_example PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "../..")
endif()