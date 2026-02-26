#include <iostream>
#include <vector>
#include <climits>
#include <algorithm>
#include <cmath>
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

// First-person camera state
struct FPSCamera {
    glm::vec3 position = glm::vec3(-300.0f, 440.0f, -450.0f);
    float yaw   = -290.0f;  // Face towards -Z initially
    float pitch  = -40.0f;
    float speed  = 50.0f;
    float sensitivity = 0.1f;
    float fov = 45.0f;

    bool mouseCaptured = false;
    double lastMouseX = 0.0, lastMouseY = 0.0;
    bool firstMouse = true;

    glm::vec3 front() const {
        float yawRad = glm::radians(yaw);
        float pitchRad = glm::radians(pitch);
        return glm::normalize(glm::vec3(
            cos(yawRad) * cos(pitchRad),
            sin(pitchRad),
            sin(yawRad) * cos(pitchRad)
        ));
    }

    glm::vec3 right() const {
        return glm::normalize(glm::cross(front(), glm::vec3(0.0f, 1.0f, 0.0f)));
    }
};

static FPSCamera camera;

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
    GLFWwindow* window = glfwCreateWindow(1024, 768, "Homogeneous", nullptr, nullptr);
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
        VoxFile voxFile = VoxReader::load("assets/voxes/pieta.vox");

        std::cout << "VOX file version: " << voxFile.version << std::endl;
        std::cout << "Number of models: " << voxFile.models.size() << std::endl;

        // Merge all sub-models with their scene graph transforms
        size_t totalVoxelCount = 0;
        for (const auto& m : voxFile.models) totalVoxelCount += m.voxels.size();
        voxels.reserve(totalVoxelCount);

        // Compute overall bounding box center for centering
        int minX = INT_MAX, minY = INT_MAX, minZ = INT_MAX;
        int maxX = INT_MIN, maxY = INT_MIN, maxZ = INT_MIN;

        for (size_t mi = 0; mi < voxFile.models.size(); ++mi) {
            const VoxelModel& model = voxFile.models[mi];
            const ModelTransform& tr = voxFile.modelTransforms[mi];
            for (const auto& v : model.voxels) {
                // VOX scene graph translation is in world space
                // Voxel local coords are [0, size), transform gives the model origin offset
                int wx = static_cast<int>(v.x) + tr.tx - model.sizeX / 2;
                int wy = static_cast<int>(v.y) + tr.ty - model.sizeY / 2;
                int wz = static_cast<int>(v.z) + tr.tz - model.sizeZ / 2;
                minX = std::min(minX, wx); maxX = std::max(maxX, wx);
                minY = std::min(minY, wy); maxY = std::max(maxY, wy);
                minZ = std::min(minZ, wz); maxZ = std::max(maxZ, wz);
            }
        }

        int centerX = (minX + maxX) / 2;
        int centerY = (minY + maxY) / 2;
        int centerZ = (minZ + maxZ) / 2;

        for (size_t mi = 0; mi < voxFile.models.size(); ++mi) {
            const VoxelModel& model = voxFile.models[mi];
            const ModelTransform& tr = voxFile.modelTransforms[mi];
            for (const auto& voxData : model.voxels) {
                int wx = static_cast<int>(voxData.x) + tr.tx - model.sizeX / 2 - centerX;
                int wy = static_cast<int>(voxData.y) + tr.ty - model.sizeY / 2 - centerY;
                int wz = static_cast<int>(voxData.z) + tr.tz - model.sizeZ / 2 - centerZ;

                const RGBAColor& paletteColor = voxFile.palette[voxData.colorIndex - 1];
                Voxel voxel(
                    wx, wy, wz,
                    paletteColor.r / 255.0f,
                    paletteColor.g / 255.0f,
                    paletteColor.b / 255.0f,
                    paletteColor.a / 255.0f
                );
                voxels.push_back(voxel);
            }
        }

        std::cout << "Successfully loaded " << voxels.size() << " voxels from "
                  << voxFile.models.size() << " sub-models" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error loading VOX file: " << e.what() << std::endl;
        return -1;
    }

    renderer.setVoxels(voxels);

    // Set clear color
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    std::cout << "\nAll libraries initialized successfully!" << std::endl;
    std::cout << "Right-click to capture mouse for camera look." << std::endl;
    std::cout << "WASD to move, Space to go up, Shift to go down." << std::endl;

    float lastFrameTime = static_cast<float>(glfwGetTime());

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Delta time
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        glfwPollEvents();

        // Right-click to toggle mouse capture
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            if (!camera.mouseCaptured) {
                camera.mouseCaptured = true;
                camera.firstMouse = true;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        } else {
            if (camera.mouseCaptured) {
                camera.mouseCaptured = false;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }

        // Mouse look (only when captured and ImGui doesn't want the mouse)
        if (camera.mouseCaptured && !io.WantCaptureMouse) {
            double mx, my;
            glfwGetCursorPos(window, &mx, &my);
            if (camera.firstMouse) {
                camera.lastMouseX = mx;
                camera.lastMouseY = my;
                camera.firstMouse = false;
            }
            float dx = static_cast<float>(camera.lastMouseX - mx) * camera.sensitivity;
            float dy = static_cast<float>(camera.lastMouseY - my) * camera.sensitivity; // inverted Y
            camera.lastMouseX = mx;
            camera.lastMouseY = my;

            camera.yaw += dx;
            camera.pitch += dy;
            camera.pitch = glm::clamp(camera.pitch, -89.0f, 89.0f);
        }

        // Keyboard movement (only when ImGui doesn't want the keyboard)
        if (!io.WantCaptureKeyboard) {
            glm::vec3 frontXZ = glm::normalize(glm::vec3(camera.front().x, 0.0f, camera.front().z));
            glm::vec3 rightDir = camera.right();
            float moveSpeed = camera.speed * deltaTime;

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                camera.position += frontXZ * moveSpeed;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                camera.position -= frontXZ * moveSpeed;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                camera.position += rightDir * moveSpeed;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                camera.position -= rightDir * moveSpeed;
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
                camera.position.y += moveSpeed;
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
                camera.position.y -= moveSpeed;
        }

        // Update renderer camera
        glm::vec3 target = camera.position + camera.front();
        renderer.setCameraPos(camera.position);
        renderer.setCameraTarget(target);
        renderer.setFOV(camera.fov);

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
        ImGui::Text("%.3f ms/frame (%.1f FPS)",
                    1000.0f / io.Framerate, io.Framerate);
        ImGui::Text("Voxel count: %d", renderer.getVoxelCount());

        ImGui::Separator();
        ImGui::Text("Camera");
        ImGui::Text("Pos: (%.1f, %.1f, %.1f)", camera.position.x, camera.position.y, camera.position.z);
        ImGui::Text("Yaw: %.1f  Pitch: %.1f", camera.yaw, camera.pitch);
        ImGui::SliderFloat("Speed", &camera.speed, 5.0f, 200.0f);
        ImGui::SliderFloat("Sensitivity", &camera.sensitivity, 0.01f, 0.5f);
        ImGui::SliderFloat("FOV", &camera.fov, 30.0f, 120.0f);

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