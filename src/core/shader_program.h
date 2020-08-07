#pragma once

#include <string>

#include "core/common.h"

enum class ShaderType : uint32_t {
    Vertex = GL_VERTEX_SHADER,
    Fragment = GL_FRAGMENT_SHADER,
    Geometry = GL_GEOMETRY_SHADER,
    TessControl = GL_TESS_CONTROL_SHADER,
    TessEvaluation = GL_TESS_EVALUATION_SHADER,
    Compute = GL_COMPUTE_SHADER
};

class ShaderProgram {
public:
    ShaderProgram();
    virtual ~ShaderProgram();

    void create();
    void addShaderFromFile(const std::string &filename, ShaderType type);
    void addShaderFromSource(const std::string &source, ShaderType type);
    void link();
    void destroy();

    void bind() { glUseProgram(programId); }
    void release() { glUseProgram(0); }

    GLint getUniformLocation(const std::string& name) {
        GLint currentProgramId;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgramId);
        if (currentProgramId != programId) {
            FatalError("Uniform variable %s i set before program is used!", name.c_str());
        }
        return glGetUniformLocation(programId, name.c_str());
    }

    void setUniformValue(const std::string &name, float v) {
        const GLint location = getUniformLocation(name);
        if (location >= 0) {
            glUniform1f(location, v);
        }
    }

    void setUniformValue(const std::string &name, const glm::vec2& v) {
        const GLint location = getUniformLocation(name);
        if (location >= 0) {
            glUniform2fv(location, 1, glm::value_ptr(v));
        }
    }

    void setUniformValue(const std::string& name, const glm::vec3& v) {
        const GLint location = getUniformLocation(name);
        if (location >= 0) {
            glUniform3fv(location, 1, glm::value_ptr(v));
        }
    }

    void setUniformValue(const std::string& name, const glm::vec4& v) {
        const GLint location = getUniformLocation(name);
        if (location >= 0) {
            glUniform4fv(location, 1, glm::value_ptr(v));
        }
    }

    void setUniformValue(const std::string& name, int v) {
        const GLint location = getUniformLocation(name);
        if (location >= 0) {
            glUniform1i(location, v);
        }
    }

    void setUniformValue(const std::string& name, const glm::ivec2& v) {
        const GLint location = getUniformLocation(name);
        if (location >= 0) {
            glUniform2iv(location, 1, glm::value_ptr(v));
        }
    }

    void setUniformValue(const std::string& name, const glm::ivec3& v) {
        const GLint location = getUniformLocation(name);
        if (location >= 0) {
            glUniform3iv(location, 1, glm::value_ptr(v));
        }
    }

    void setUniformValue(const std::string& name, const glm::ivec4& v) {
        const GLint location = getUniformLocation(name);
        if (location >= 0) {
            glUniform4iv(location, 1, glm::value_ptr(v));
        }
    }

    void setUniformValue(const std::string& name, const glm::mat4& m) {
        const GLint location = getUniformLocation(name);
        if (location >= 0) {
            glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(m));
        }
    }

    void setUniformValueArray(const std::string& name, const float* values, int count) {
        const GLint location = getUniformLocation(name);
        if (location >= 0) {
            glUniform1fv(location, count, values);
        }
    }

    void setUniformValueArray(const std::string& name, const glm::vec2* values, int count) {
        const GLint location = getUniformLocation(name);
        if (location >= 0) {
            glUniform2fv(location, count, (GLfloat*)values);
        }
    }

    void setUniformValueArray(const std::string& name, const glm::vec3* values, int count) {
        const GLuint location = getUniformLocation(name);
        if (location >= 0) {
            glUniform3fv(location, count, (GLfloat*)values);
        }
    }

    void setUniformValueArray(const std::string& name, const glm::vec4* values, int count) {
        const GLint location = getUniformLocation(name);
        if (location >= 0) {
            glUniform4fv(location, count, (GLfloat*)values);
        }
    }

private:
    GLuint programId;
};
