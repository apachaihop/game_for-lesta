cmake_minimum_required(VERSION 3.16)

if (SDL3_SRC_DIR)
    add_subdirectory(
            # Specifies the directory of the CMakeLists.txt file.
            ${SDL3_SRC_DIR}
            # Specifies the directory for the build outputs.
            ${CMAKE_CURRENT_BINARY_DIR}/SDL_${ANDROID_ABI})
endif ()

if (GLM_SRC_DIR)
    add_subdirectory(

            ${GLM_SRC_DIR}

            ${CMAKE_CURRENT_BINARY_DIR}/GLM_${ANDROID_ABI})
endif ()

project(game)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    add_library(game SHARED)
else ()
    add_executable(game)
endif ()

target_sources(
        game
        PRIVATE
        game/game.cpp
        eng/glad/glad.c
        eng/glad/glad.h
        eng/khr/khrplatform.h
        eng/engine.cxx
        eng/engine.hxx
        eng/stb.cxx
        eng/shader.cxx
        eng/texture.cxx
        eng/sprite.cxx)

set_target_properties(game PROPERTIES ENABLE_EXPORTS TRUE)

target_compile_features(game PRIVATE cxx_std_17)

set(CMAKE_CXX_STANDARD 17)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    find_library(SDL3_LIB NAMES SDL3)
else ()
    find_package(SDL3 REQUIRED)
    find_package(glm REQUIRED)

endif ()


if (${CMAKE_SYSTEM_NAME} STREQUAL "Android")

    target_link_libraries(
            game
            PRIVATE SDL3::SDL3
            glm::glm
            android
            log
            EGL
            GLESv3)
else ()
    if (WIN32)
        target_link_libraries(game PRIVATE SDL3::SDL3-static glm::glm)
    else ()
        target_link_libraries(game PRIVATE SDL3::SDL3-shared glm::glm)
    endif ()
endif ()


add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/eng/imgui)

target_link_libraries(game PRIVATE imgui)

