cmake_minimum_required(VERSION 3.10)

project(ImpossibleHexagon)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(APPLE)
    add_compile_definitions(GL_SILENCE_DEPRECATION)
endif()

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

if(NOT APPLE)
    find_package(GLEW REQUIRED)
endif()

include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/shaders)

add_executable(imposible.out src/main.cpp src/penrose.cpp src/neckercube.cpp src/penrose_blocks.cpp src/arch.cpp)

if(APPLE)
    # Mac linking (No GLEW required)
    target_link_libraries(imposible.out PRIVATE
        OpenGL::GL
        glfw
    )
else()
    # Linux and Windows linking (Requires GLEW)
    target_link_libraries(imposible.out PRIVATE
        OpenGL::GL
        glfw
        GLEW::GLEW
    )
endif()

