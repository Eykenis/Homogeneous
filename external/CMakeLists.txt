# External dependencies configuration

# ============================================================================
# GLFW
# ============================================================================
add_subdirectory(GLFW/glfw-3.4)

# Create interface library for GLFW
add_library(GLFW::GLFW ALIAS glfw)

# ============================================================================
# GLAD (header-only + source for loader)
# ============================================================================
set(GLAD_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/GLAD/src/glad.c
)

set(GLAD_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/GLAD/include/glad/glad.h
)

add_library(glad STATIC ${GLAD_SOURCES})
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/GLAD/include)
add_library(GLAD::GLAD ALIAS glad)

# ============================================================================
# GLM (header-only)
# ============================================================================
set(GLM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/GLM)
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ${GLM_INCLUDE_DIR})
add_library(glm::glm ALIAS glm)

# ============================================================================
# ImGUI
# ============================================================================
set(IMGUI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/backends/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/backends/imgui_impl_opengl3.cpp
)

set(IMGUI_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/imgui.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/imgui_internal.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/imconfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/imstb_rectpack.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/imstb_textedit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/imstb_truetype.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/backends/imgui_impl_glfw.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/backends/imgui_impl_opengl3.h
)

add_library(imgui STATIC ${IMGUI_SOURCES} ${IMGUI_HEADERS})
target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/backends
)
target_link_libraries(imgui PUBLIC GLFW::GLFW GLAD::GLAD)
add_library(ImGui::ImGui ALIAS imgui)
