cmake_minimum_required(VERSION 3.0)
project (terrain_editor)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

set(HEADER_FILES
	source/GLWidget.h
)

set(SOURCE_FILES
	source/GLWidget.cpp
	source/main.cpp
)

find_package(Qt5Widgets)
find_package(Qt5OpenGL)
find_package(flyEngine REQUIRED)

include_directories(source/ ${OpenCV_DIRS} ${FLY_DIRS} ${ANT_DIR})
add_executable(terrain_editor ${HEADER_FILES} ${SOURCE_FILES} )

target_link_libraries(terrain_editor Qt5::Widgets Qt5::OpenGL ${FLY_LIBS})

if (WIN32)
set_target_properties(terrain_editor PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "@CMAKE_CURRENT_LIST_DIR@/..")
endif()