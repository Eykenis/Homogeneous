/**
 * Shader Class
 *
 * Encapsulates OpenGL shader operations:
 * - Reading shader source from files
 * - Compiling vertex and fragment shaders
 * - Linking shader programs
 * - Activating shader programs
 */

#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
public:
    // Shader program ID
    unsigned int ID;

    /**
     * Constructor: Reads and builds the shader
     * @param vertexPath Path to vertex shader file
     * @param fragmentPath Path to fragment shader file
     */
    Shader(const char* vertexPath, const char* fragmentPath) {
        // Read shader sources
        std::string vertexCode = readFile(vertexPath);
        std::string fragmentCode = readFile(fragmentPath);
        const char* vertexSource = vertexCode.c_str();
        const char* fragmentSource = fragmentCode.c_str();

        // Compile shaders
        unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
        unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

        if (vertexShader == 0 || fragmentShader == 0) {
            ID = 0;
            return;
        }

        // Create program
        ID = glCreateProgram();
        glAttachShader(ID, vertexShader);
        glAttachShader(ID, fragmentShader);
        glLinkProgram(ID);

        // Check for linking errors
        checkLinkErrors();

        // Delete shaders (they're now part of the program)
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    /**
     * Activate the shader program
     */
    void use() const {
        glUseProgram(ID);
    }

    /**
     * Utility uniform setter functions
     */
    void setBool(const std::string& name, bool value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }

    void setInt(const std::string& name, int value) const {
        int loc = glGetUniformLocation(ID, name.c_str());
        if (loc == -1) {
            std::cerr << "WARNING: Uniform '" << name << "' not found in shader program " << ID << std::endl;
        }
        glUniform1i(loc, value);
    }

    void setFloat(const std::string& name, float value) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setVec3(const std::string& name, float x, float y, float z) const {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }

    void setMatrix4fv(const std::string& name, const float* matrix) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, matrix);
    }

    /**
     * Check if shader was created successfully
     */
    bool isValid() const {
        return ID != 0;
    }

    /**
     * Destructor: Deletes the shader program
     */
    ~Shader() {
        glDeleteProgram(ID);
    }

private:
    /**
     * Read shader source from file
     */
    std::string readFile(const char* filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "ERROR: Failed to open shader file: " << filepath << std::endl;
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        std::cout << "Loaded shader: " << filepath << " (" << content.size() << " bytes)" << std::endl;
        return content;
    }

    /**
     * Compile a shader from source
     */
    unsigned int compileShader(unsigned int type, const char* source) {
        unsigned int shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, NULL);
        glCompileShader(shader);

        // Check for compilation errors
        int success;
        char infoLog[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cerr << "ERROR: Shader compilation failed\n" << infoLog << std::endl;
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    /**
     * Check for program linking errors
     */
    void checkLinkErrors() {
        int success;
        char infoLog[512];
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(ID, 512, NULL, infoLog);
            std::cerr << "ERROR: Shader program linking failed\n" << infoLog << std::endl;
            glDeleteProgram(ID);
            ID = 0;
        }
    }
};

#endif
