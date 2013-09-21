PROJECT( eyelike_project )
FIND_PACKAGE( OpenCV REQUIRED )
cmake_minimum_required(VERSION 2.8)

# debug or relase:
# todo: make it into a cli option setting as well ?
set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_BUILD_TYPE Release)


SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_BINARY_DIR ${PROJECT_BINARY_DIR}/bin)

add_subdirectory(src)
