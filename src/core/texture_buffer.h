#pragma once

#include "core/common.h"

class TextureBuffer {
public:
    TextureBuffer(size_t size, GLenum internalFormat, GLenum usage);
    virtual ~TextureBuffer();

    void bind(int id = 0);
    void release(int id = 0);
    void setData(void *data);
    void destroy();

    size_t size() const { return size_; }

private:
    void initialize();

    GLuint texId = 0u, bufId = 0u;
    size_t size_;
    GLenum internalFormat, usage;
};
