cmake_minimum_required(VERSION 3.18)

project(Homogeneous LANGUAGES CXX CUDA)

# ============================================================================
# C++ and CUDA Standards
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 14)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# CUDA Architecture for RTX 4060 (Ada Lovelace)
set(CMAKE_CUDA_ARCHITECTURES 89)

# ============================================================================
# Compiler Settings
# ============================================================================
if(MSVC)
    # MSVC specific flags
    add_compile_options(/W4)
    add_compile_options("$<$<COMPILE_LANGUAGE:CUDA>:SHELL:--compiler-options /W4>")
endif()

# ============================================================================
# OpenGL
# ============================================================================
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

# ============================================================================
# External Dependencies
# ============================================================================
add_subdirectory(external)

# ============================================================================
# Main Executable
# ============================================================================
set(SOURCES
    src/main.cpp
    src/voxel.cpp
    src/shader.cpp
    src/voxel_renderer.cpp
    src/vox_reader.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
    GLFW::GLFW
    GLAD::GLAD
    glm::glm
    ImGui::ImGui
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ============================================================================
# Copy Assets to Build Directory
# ============================================================================
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMENT "Copying assets to build directory"
)