cmake_minimum_required(VERSION 3.0)
project (qt_example)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

set(HEADER_FILES
	include/GLWidget.h
	include/AntWrapper.h
)

set(SOURCE_FILES
	source/GLWidget.cpp
	source/main.cpp
	source/AntWrapper.cpp
)

find_package(Qt5Widgets)
find_package(Qt5OpenGL)
find_package(flyEngine REQUIRED)
set (ANT_DIR "" CACHE PATH "")
set (ANT_LIB "" CACHE FILEPATH "")

include_directories(include/ ${OpenCV_DIRS} ${FLY_DIRS} ${ANT_DIR})
add_executable(qt_example ${HEADER_FILES} ${SOURCE_FILES} )

target_link_libraries(qt_example Qt5::Widgets Qt5::OpenGL ${FLY_LIBS} ${ANT_LIB})

if (WIN32)
set_target_properties(qt_example PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "@CMAKE_CURRENT_LIST_DIR@/../..")
endif()