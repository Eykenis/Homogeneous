cmake_minimum_required(VERSION 3.28)

# Find CUDA 13.1 (required, no fallback)
find_package(CUDAToolkit REQUIRED)

# Verify CUDA version
if(CUDAToolkit_VERSION VERSION_LESS 13.1)
    message(FATAL_ERROR "CUDA 13.1 or higher is required. Found version: ${CUDAToolkit_VERSION}")
endif()

project(Homogeneous
    VERSION 0.1.0
    LANGUAGES CXX CUDA C
)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CUDA Standard
set(CMAKE_CUDA_STANDARD 14)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_EXTENSIONS OFF)
set(CMAKE_CUDA_ARCHITECTURES 89)

# Find OpenGL (required for GLFW and GLAD)
find_package(OpenGL REQUIRED)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/external/GLM)

# Add subdirectories
add_subdirectory(external)
add_subdirectory(tests)

# ============================================================================
# Hello World: Triangle Renderer
# ============================================================================
add_executable(hello_triangle
    src/hello_triangle.cpp
    src/scene.cpp
    src/texture.cpp
)

target_link_libraries(hello_triangle PRIVATE
    OpenGL::GL
    GLFW::GLFW
    GLAD::GLAD
)
