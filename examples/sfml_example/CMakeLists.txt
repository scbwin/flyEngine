cmake_minimum_required(VERSION 3.0)
project (sfml_example)

set(SOURCES
source/main.cpp
)

list(APPEND CMAKE_MODULE_PATH "C:/Program Files/SFML/cmake/Modules/")

find_package(OpenCV REQUIRED)
find_package(flyEngine REQUIRED)
find_package(SFML COMPONENTS window system graphics)

include_directories (include ${OpenCV_DIRS} ${FLY_DIRS} ${SFML_INCLUDE_DIR})
add_executable(sfml_example ${SOURCES})

target_link_libraries(sfml_example ${FLY_LIBS} ${SFML_LIBRARIES})

if (WIN32)
set_target_properties(sfml_example PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "@CMAKE_CURRENT_LIST_DIR@/../..")
endif()