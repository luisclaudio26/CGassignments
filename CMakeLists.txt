cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
cmake_policy(SET CMP0015 NEW)
project(AlmostGL)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Headers
include_directories(include)
include_directories(${OPENGL_INCLUDE_DIR})
include_directories(3rdparty/nanovg/src)

#Source files
file(GLOB SOURCES "src/*.cpp")

#Link libraries
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)

set(LIBS nanogui glfw ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})

add_executable(AlmostGL ${SOURCES})
target_link_libraries(AlmostGL ${LIBS})
