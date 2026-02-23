#include <iostream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "voxel_renderer.h"
#include "voxel.h"
#include "vox_reader.h"

int main()
{
    std::cout << "Hello, Homogeneous!" << std::endl;
    std::cout << "Initializing GLFW..." << std::endl;

    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW for OpenGL 4.6 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(640, 480, "Homogeneous - Voxel Renderer", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Print OpenGL version
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    std::cout << "ImGui initialized successfully" << std::endl;

    // Initialize VoxelRenderer
    VoxelRenderer renderer;
    renderer.init();

    // Load voxel model
    std::vector<Voxel> voxels;
    try {
        std::cout << "Loading .vox..." << std::endl;
        VoxFile voxFile = VoxReader::load("assets/voxes/aiz.vox");

        std::cout << "VOX file version: " << voxFile.version << std::endl;
        std::cout << "Number of models: " << voxFile.models.size() << std::endl;

        if (!voxFile.models.empty()) {
            const VoxelModel& model = voxFile.models[0];
            std::cout << "Model dimensions: " << model.sizeX << "x"
                      << model.sizeY << "x" << model.sizeZ << std::endl;
            std::cout << "Number of voxels: " << model.voxels.size() << std::endl;

            // Convert VOX format voxels to rendering Voxel objects
            voxels.reserve(model.voxels.size());
            for (const auto& voxData : model.voxels) {
                // Get color from palette
                const RGBAColor& paletteColor = voxFile.palette[voxData.colorIndex];

                // Convert to normalized color
                glm::vec4 color(
                    paletteColor.r / 255.0f,
                    paletteColor.g / 255.0f,
                    paletteColor.b / 255.0f,
                    paletteColor.a / 255.0f
                );

                // Create Voxel object (center the model by subtracting half dimensions)
                Voxel voxel(
                    static_cast<int>(voxData.x) - model.sizeX / 2,
                    static_cast<int>(voxData.y) - model.sizeY / 2,
                    static_cast<int>(voxData.z) - model.sizeZ / 2,
                    color.r, color.g, color.b, color.a
                );
                voxels.push_back(voxel);
            }

            std::cout << "Successfully loaded " << voxels.size() << " voxels" << std::endl;
        } else {
            std::cerr << "Warning: VOX file contains no models" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading VOX file: " << e.what() << std::endl;
        std::cerr << "Falling back to test voxels..." << std::endl;

        // Fallback: create simple test voxels
        for (int x = -1; x <= 1; x++)
            for (int z = -1; z <= 1; z++)
                voxels.emplace_back(x, 0, z, 0.3f, 0.8f, 0.3f, 1.0f);
        voxels.emplace_back(0, 1, 0, 1.0f, 0.5f, 0.2f, 1.0f);
    }

    renderer.setVoxels(voxels);

    // Camera settings (adjusted for larger model)
    glm::vec3 cameraPos(50.0f, 50.0f, 80.0f);
    glm::vec3 cameraTarget(0.0f, 0.0f, 0.0f);
    float fov = 45.0f;

    renderer.setCameraPos(cameraPos);
    renderer.setCameraTarget(cameraTarget);
    renderer.setFOV(fov);

    // Test GLM
    glm::vec3 testVec(1.0f, 2.0f, 3.0f);
    glm::mat4 testMat = glm::translate(glm::mat4(1.0f), testVec);
    std::cout << "GLM test vector: (" << testVec.x << ", " << testVec.y << ", " << testVec.z << ")" << std::endl;

    // Set clear color
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    std::cout << "\nAll libraries initialized successfully!" << std::endl;
    std::cout << "Press ESC to close the window..." << std::endl;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll events
        glfwPollEvents();

        // Get window size
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render voxel using ray marching
        renderer.render(width, height);

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui controls
        ImGui::Begin("Voxel Ray Marching");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                    1000.0f / io.Framerate, io.Framerate);
        ImGui::Text("Voxel count: %d", renderer.getVoxelCount());

        ImGui::Separator();
        ImGui::Text("Camera Controls");
        ImGui::SliderFloat3("Camera Position", &cameraPos.x, -100.0f, 100.0f);
        ImGui::SliderFloat3("Camera Target", &cameraTarget.x, -50.0f, 50.0f);
        ImGui::SliderFloat("FOV", &fov, 30.0f, 90.0f);

        renderer.setCameraPos(cameraPos);
        renderer.setCameraTarget(cameraTarget);
        renderer.setFOV(fov);

        ImGui::Separator();
        if (ImGui::Button("Close"))
            glfwSetWindowShouldClose(window, true);
        ImGui::End();

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers
        glfwSwapBuffers(window);

        // Check for ESC key
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Goodbye!" << std::endl;
    return 0;
}