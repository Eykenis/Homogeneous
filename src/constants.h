/**
 * Constants Header
 *
 * Defines all mathematical and rendering constants used in the project
 * Also configures GLM library for OpenGL compatibility
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

// Configure GLM for OpenGL
#define GLM_FORCE_RADIANS          // Use radians by default
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Pi constant
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#endif
