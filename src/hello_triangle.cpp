/**
 * Voxel Scene Renderer
 *
 * This program demonstrates voxel scene rendering using the Scene class:
 * - 16x16x256 voxel world
 * - Face culling optimization for performance
 * - Camera controls for navigation
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "shader.h"
#include "camera.h"
#include "scene.h"

// Window dimensions
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

/**
 * GLFW framebuffer size callback
 */
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// ============================================================================
// Global camera control variables (passed to callbacks)
// ============================================================================
float cameraYaw = -90.0f;
float cameraPitch = 0.0f;
float cameraSpeed = 5.0f;
float mouseSensitivity = 0.1f;
bool firstMouse = true;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;

// Camera direction vectors (front, right) - store as float arrays to avoid GLM static init
float cameraFront[3] = {0.0f, 0.0f, -1.0f};
float cameraRight[3] = {1.0f, 0.0f, 0.0f};

/**
 * GLFW mouse position callback - for camera rotation
 */
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    cameraYaw += xoffset;
    cameraPitch += yoffset;

    if (cameraPitch > 89.0f) cameraPitch = 89.0f;
    if (cameraPitch < -89.0f) cameraPitch = -89.0f;
}

int main() {
    std::cout << "Starting Voxel Scene Renderer..." << std::endl;

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW: use OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Voxel Scene Renderer", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Set callback for window resize
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);


    // Load OpenGL function pointers with GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    // Enable depth test for proper 3D rendering
    glEnable(GL_DEPTH_TEST);

    // Enable backface culling for performance
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Create and initialize Scene
    Scene *scene = new Scene();
    scene->init();

    std::cout << "\n=== Voxel Scene Renderer ===" << std::endl;
    std::cout << "Scene dimensions: " << Scene::WIDTH << "x" << Scene::HEIGHT << "x" << Scene::DEPTH << std::endl;
    std::cout << "Total blocks: " << Scene::TOTAL_BLOCKS << std::endl;
    std::cout << "\nControls:" << std::endl;
    std::cout << "  WASD - Move horizontally (XZ plane)" << std::endl;
    std::cout << "  Space - Move up" << std::endl;
    std::cout << "  Shift - Move down" << std::endl;
    std::cout << "  Mouse - Look around" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;

    // Create voxel shader
    const char* vertexPath = "../../../assets/shaders/voxel.vert";
    const char* fragmentPath = "../../../assets/shaders/voxel.frag";

    Shader shader(vertexPath, fragmentPath);

    if (!shader.isValid()) {
        std::cerr << "Failed to create shader" << std::endl;
        return -1;
    }

    // Set mouse input mode (capture mouse for camera control)
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    // Create camera: position to view the entire scene
    Camera camera(8.0f, 8.0f, 12.0f,
                  8.0f, 4.0f, 0.0f,
                  0.0f, 1.0f, 0.0f);

    float aspect = (float)SCR_WIDTH / (float)SCR_HEIGHT;
    camera.setProjection(75.0f, aspect, 0.1f, 100.0f);

    float lastFrameTime = (float)glfwGetTime();

    // Render Loop
    while (!glfwWindowShouldClose(window)) {
        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        // Process keyboard input for camera movement
        float moveSpeed = cameraSpeed * deltaTime;

        // Create GLM vectors from float arrays
        glm::vec3 front(cameraFront[0], cameraFront[1], cameraFront[2]);
        glm::vec3 right(cameraRight[0], cameraRight[1], cameraRight[2]);

        // WASD - horizontal movement
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.position += front * moveSpeed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.position -= front * moveSpeed;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.position -= right * moveSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.position += right * moveSpeed;

        // Space - move up
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            camera.position.y += moveSpeed;

        // Shift - move down
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
            camera.position.y -= moveSpeed;

        // Update camera front vector based on yaw and pitch
        front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front.y = sin(glm::radians(cameraPitch));
        front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front = glm::normalize(front);

        // Update global cameraFront array
        cameraFront[0] = front.x;
        cameraFront[1] = front.y;
        cameraFront[2] = front.z;

        // Calculate right vector
        right = glm::normalize(glm::cross(front, camera.up));
        cameraRight[0] = right.x;
        cameraRight[1] = right.y;
        cameraRight[2] = right.z;

        // Update camera target
        camera.target = camera.position + front;

        // Update camera matrices
        camera.updateMatrices();

        // Process input
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        // Clear screen
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render the voxel scene
        scene->render(shader, camera);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
