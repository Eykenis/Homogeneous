# Enable testing
enable_testing()

# Create test executable (CUDA is required)
add_executable(homogeneous_tests
    test_main.cpp
    test_cuda.cu
)

# Link libraries
target_link_libraries(homogeneous_tests PRIVATE
    OpenGL::GL
    GLFW::GLFW
    GLAD::GLAD
)

# Register tests
add_test(NAME all_tests COMMAND homogeneous_tests)
