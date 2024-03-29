#pragma once

#include <string>

#include "common.h"

class Texture {
public:
    // PUBLIC methods
    Texture();
    explicit Texture(const std::string &filename, bool generateMipMap = false);
    Texture(int width, int height, GLenum internalFormat = GL_RGBA8,
            GLenum format = GL_RGBA, GLenum type = GL_UNSIGNED_BYTE);
    Texture(int width, int height);
    virtual ~Texture();

    void load(const std::string &filename, bool generateMipMap = false);

    void clear(const glm::vec4 &color);

    void bind(GLuint i = 0) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textureId);
    }

    void unbind(GLuint i = 0) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void destroy() {
        if (textureId != 0) {
            glDeleteTextures(1, &textureId);
            textureId = 0;
        }
    }

    GLuint getId() const { return textureId; }
    int width() const { return width_; }
    int height() const { return height_; }

private:
    // PRIVATE parameters
    int width_, height_, channels_;
    GLenum internalFormat_, format_, type_;
    GLuint textureId = 0;
};
