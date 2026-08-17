cmake_minimum_required(VERSION 3.16)
project(EvlyMC LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(glm QUIET)

add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

add_executable(EvlyMC
    src/main.cpp
    src/Window.cpp
    src/Camera.cpp
    src/Tile.cpp
    src/Chunk.cpp
    src/World.cpp
    src/Player.cpp
)

target_include_directories(EvlyMC PRIVATE src external)
target_link_libraries(EvlyMC PRIVATE glad glfw OpenGL::GL)

if(TARGET glm::glm)
    target_link_libraries(EvlyMC PRIVATE glm::glm)
endif()

file(COPY shaders/vertex.glsl shaders/fragment.glsl
     DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)

if(UNIX AND NOT APPLE)
    target_link_libraries(EvlyMC PRIVATE dl pthread)
endif()
