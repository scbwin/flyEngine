cmake_minimum_required(VERSION 3.0)
project (marching_cubes_test)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

set(HEADER_FILES
	source/MCWidget.h
  ../common/BaseQtScene.h
)

set(SOURCE_FILES
	source/MCWidget.cpp
	source/main.cpp
  ../common/BaseQtScene.cpp
)

find_package(Qt5Widgets)
find_package(Qt5OpenGL)
find_package(flyEngine REQUIRED)
set (ANT_DIR "" CACHE PATH "")
set (ANT_LIB "" CACHE FILEPATH "")

include_directories(source/ ${OpenCV_DIRS} ${FLY_DIRS} ${ANT_DIR} ../common/)
add_executable(marching_cubes_test ${HEADER_FILES} ${SOURCE_FILES} )

target_link_libraries(marching_cubes_test Qt5::Widgets Qt5::OpenGL ${FLY_LIBS} ${ANT_LIB})

if (WIN32)
set_target_properties(marching_cubes_test PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "@CMAKE_CURRENT_LIST_DIR@/..")
endif()