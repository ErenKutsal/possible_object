cmake_minimum_required(VERSION 3.10)

project(ImpossibleHexagon)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_definitions(GL_SILENCE_DEPRECATION)

find_package(OpenGL REQUIRED)

find_package(glfw3 REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/util ${CMAKE_SOURCE_DIR}/shaders)

add_executable(posible.out src/main.cpp util/InitShader.cpp)

# Link the required libraries to your executable
target_link_libraries(posible.out PRIVATE 
    OpenGL::GL 
    glfw
    # GLEW::GLEW
)

add_executable(imposible.out src/main2.cpp util/InitShader.cpp)

target_link_libraries(imposible.out PRIVATE 
    OpenGL::GL 
    glfw
)