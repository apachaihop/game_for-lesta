cmake_minimum_required(VERSION 3.25)
project(opengl_window)

find_package(SDL3 REQUIRED)
find_package(glm REQUIRED)


set(CMAKE_CXX_STANDARD 17)

add_executable(
        opengl_window
        game.cpp
        glad/glad.c
        glad/glad.h
        khr/khrplatform.h
        engine.cxx
        engine.hxx
        stb.cxx
        shader.cxx
        texture.cxx
        sprite.cxx)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/imgui)

target_link_libraries(opengl_window PRIVATE imgui)

if(WIN32)
    target_link_libraries(opengl_window PRIVATE SDL3::SDL3-static glm::glm)
else()
    target_link_libraries(opengl_window PRIVATE SDL3::SDL3-shared glm::glm)
endif()