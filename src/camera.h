/**
 * Camera Class
 *
 * Encapsulates camera view transformations:
 * - Position, target (look-at point), up direction
 * - View matrix calculation (look-at)
 * - Perspective projection matrix
 */

#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera {
public:
    // Camera properties
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;

    // Projection properties
    float fov;          // Field of view in radians
    float aspectRatio;
    float nearPlane;
    float farPlane;

    // Matrices
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::mat4 viewProjectionMatrix;

    /**
     * Constructor - initialize camera with default values
     */
    Camera()
        : fov(glm::radians(45.0f)), aspectRatio(4.0f / 3.0f),
          nearPlane(0.1f), farPlane(100.0f) {

        // Default camera position (looking from +X, +Y, +Z towards origin)
        position = glm::vec3(2.0f, 2.0f, 2.0f);
        target = glm::vec3(0.0f, 0.0f, 0.0f);
        up = glm::vec3(0.0f, 1.0f, 0.0f);

        updateMatricesInternal();
    }

    /**
     * Constructor - initialize camera with custom position, target, and up vector
     */
    Camera(float posX, float posY, float posZ,
           float targetX, float targetY, float targetZ,
           float upX, float upY, float upZ)
        : fov(glm::radians(45.0f)), aspectRatio(4.0f / 3.0f),
          nearPlane(0.1f), farPlane(100.0f) {

        position = glm::vec3(posX, posY, posZ);
        target = glm::vec3(targetX, targetY, targetZ);
        up = glm::vec3(upX, upY, upZ);

        updateMatricesInternal();
    }

    /**
     * Set camera position
     */
    void setPosition(float x, float y, float z) {
        position = glm::vec3(x, y, z);
        updateMatricesInternal();
    }

    /**
     * Set camera target (look-at point)
     */
    void setTarget(float x, float y, float z) {
        target = glm::vec3(x, y, z);
        updateMatricesInternal();
    }

    /**
     * Set camera up vector
     */
    void setUp(float x, float y, float z) {
        up = glm::vec3(x, y, z);
        updateMatricesInternal();
    }

    /**
     * Set projection parameters
     */
    void setProjection(float fovDegrees, float aspect, float near, float far) {
        fov = glm::radians(fovDegrees);
        aspectRatio = aspect;
        nearPlane = near;
        farPlane = far;
        updateMatricesInternal();
    }

    /**
     * Get the view-projection matrix
     */
    const float* getViewProjectionMatrix() const {
        return glm::value_ptr(viewProjectionMatrix);
    }

    /**
     * Get the view matrix
     */
    const float* getViewMatrix() const {
        return glm::value_ptr(viewMatrix);
    }

    /**
     * Get the projection matrix
     */
    const float* getProjectionMatrix() const {
        return glm::value_ptr(projectionMatrix);
    }

    /**
     * Public method to update all matrices
     */
    void updateMatrices() {
        viewMatrix = glm::lookAt(position, target, up);
        projectionMatrix = glm::perspective(fov, aspectRatio, nearPlane, farPlane);
        viewProjectionMatrix = projectionMatrix * viewMatrix;
    }

private:
    /**
     * Update all matrices when camera parameters change
     */
    void updateMatricesInternal() {
        viewMatrix = glm::lookAt(position, target, up);
        projectionMatrix = glm::perspective(fov, aspectRatio, nearPlane, farPlane);
        viewProjectionMatrix = projectionMatrix * viewMatrix;
    }
};

#endif
