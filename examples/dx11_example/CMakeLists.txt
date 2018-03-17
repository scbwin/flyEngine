cmake_minimum_required(VERSION 3.0)
project (dx11_example)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

set(HEADER_FILES
	include/DX11App.h
)

set(SOURCE_FILES
	source/main.cpp
	source/DX11App.cpp
)

set (ANT_DIR "" CACHE PATH "")
set (ANT_LIB "" CACHE FILEPATH "")
find_package(OpenCV REQUIRED)
find_package(flyEngine REQUIRED)

include_directories(include/  ${OpenCV_DIRS} ${ANT_DIR} ${FLY_DIRS})
add_executable(dx11_example ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(dx11_example ${ANT_LIB} ${FLY_LIBS})

if (WIN32)
set_target_properties(dx11_example PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "@CMAKE_CURRENT_LIST_DIR@/../..")
endif()