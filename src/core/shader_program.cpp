#include "shader_program.h"

#include <iostream>
#include <fstream>

ShaderProgram::ShaderProgram() {
}

ShaderProgram::~ShaderProgram() {
}

void ShaderProgram::create() {
    programId = glCreateProgram();
}

void ShaderProgram::addShaderFromFile(const std::string& filename, ShaderType type) {
    std::ifstream reader(filename.c_str(), std::ios::in);
    if (reader.fail()) {
        throw std::runtime_error("Failed to open shader source: " + filename);
    }

    std::string code;
    reader.seekg(0, std::ios::end);
    code.reserve(reader.tellg());
    reader.seekg(0, std::ios::beg);    
    code.assign(std::istreambuf_iterator<char>(reader), std::istreambuf_iterator<char>());
    reader.close();

    addShaderFromSource(code, type);
}

void ShaderProgram::addShaderFromSource(const std::string& source, ShaderType type) {
    GLuint shaderId = glCreateShader((GLuint)type);

    const char *codePtr = source.c_str();
    glShaderSource(shaderId, 1, &codePtr, nullptr);
    glCompileShader(shaderId);

    GLint compileStatus;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE) {
        GLint logLength;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            GLsizei length;
            std::string errMsg;
            errMsg.resize((size_t)logLength);
            glGetShaderInfoLog(shaderId, logLength, &length, &errMsg[0]);
            fprintf(stderr, "%s\n", errMsg.c_str());
            fprintf(stderr, "%s\n", source.c_str());
        }
        FatalError("Failed to compile a shader!");
    }

    glAttachShader(programId, shaderId);
}

void ShaderProgram::link() {
    glLinkProgram(programId);

    GLint linkStatus;
    glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);

    if (linkStatus == GL_FALSE) {
        GLint logLength;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            GLsizei length;
            std::string errMsg;
            errMsg.resize((size_t)logLength);
            glGetProgramInfoLog(programId, logLength, &length, &errMsg[0]);
            fprintf(stderr, "%s\n", errMsg.c_str());
        }
        FatalError("Failed to link shaders!");
    }
}

void ShaderProgram::destroy() {
    if (programId != 0) {
        glDeleteProgram(programId);
    }
}
