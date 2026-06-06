cmake_minimum_required(VERSION 3.14)

project(ImpossibleObjects)

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

include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.5
)
FetchContent_MakeAvailable(imgui)

# stb_image — single-header PNG loader (vendored via FetchContent, header-only).
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(stb)
include_directories(${stb_SOURCE_DIR})

add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC glfw OpenGL::GL)

include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/shaders)

add_executable(
    imposible.out
    src/main.cpp
    src/impossible_polygon.cpp
    src/polygon_bg.cpp          # Bloom pipeline + interactive backgrounds (slot 0)
    # OBJ-loaded shapes (slots 1-5, 0-indexed 1-5)
    src/penrose.cpp
    src/neckercube.cpp
    src/penrose_blocks.cpp
    src/arch.cpp
    src/reutersvard.cpp
    # Variant shapes — same OBJ category as one of the OBJ slots above but
    # rendered with different proportions / palette / starting pose.
    src/penrose_blocks_proc.cpp
    src/arch_proc.cpp
    src/ui.cpp
)

if(APPLE)
    target_link_libraries(imposible.out PRIVATE
        OpenGL::GL
        glfw
        imgui_lib
    )
else()
    target_link_libraries(imposible.out PRIVATE
        OpenGL::GL
        glfw
        GLEW::GLEW
        imgui_lib
    )
endif()
