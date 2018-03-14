cmake_minimum_required(VERSION 3.0)
project (opengl_example)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Find the QtWidgets library
find_package(Qt5Widgets)
find_package(Qt5OpenGL)

file (GLOB SOURCE_FILES
	"source/*.cpp"
)

file (GLOB HEADER_FILES
	"../engine/include/*.h"
	"include/*.h"
)

set(GLM_DIR "" CACHE PATH "")
IF (WIN32)
find_package(GLEW REQUIRED)
ENDIf()
find_package(OpenGL)
find_package(assimp REQUIRED)
find_package(OpenCV REQUIRED)
set(SOIL_DIR "" CACHE PATH "")
set (SOIL_LIB "" CACHE FILEPATH "")
set(GLFW_DIR "" CACHE PATH "")
set (GLFW_LIB "" CACHE FILEPATH "")
set (FLY_LIB_DEBUG "" CACHE FILEPATH "")
set (FLY_LIB_RELEASE "" CACHE FILEPATH "")

include_directories(../engine/include/ include/ ${GLFW_DIR} ${SOIL_DIR} ${GLM_DIR} ${GLEW_INCLUDE_DIRS} ${ASSIMP_INCLUDE_DIRS} ${OpenCV_DIRS})
add_executable(opengl_example ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(opengl_example Qt5::Widgets Qt5::OpenGL ${GLFW_LIB} ${SOIL_LIB} ${GLEW_LIBRARIES} ${GLFW_LIB} ${OPENGL_gl_LIBRARY} ${ASSIMP_LIBRARY_DIRS}/${ASSIMP_LIBRARIES}.lib  ${OpenCV_LIBRARIES})
target_link_libraries(opengl_example 
	debug ${FLY_LIB_DEBUG} optimized ${FLY_LIB_RELEASE}
)

IF (WIN32)
set_target_properties(opengl_example PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "../..")
ENDIf()