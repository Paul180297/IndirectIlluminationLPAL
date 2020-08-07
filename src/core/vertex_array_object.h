#pragma once

#include <string>

#include "core/common.h"

class VertexArrayObject {
public:
    VertexArrayObject();
    virtual ~VertexArrayObject();

    void create();
    void addMeshFromFile(const std::string &filename);
    void draw(GLenum mode);
    void destroy();

private:
    GLuint vaoId, vboId, iboId;
    GLsizei bufferSize;
};
