#include "direct_volume.h"
#include <vector>


#include "volume_texture.h"

namespace {

const float texcoord[8][3] = {
    { 0.0f, 0.0f, 0.0f },
    { 1.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f },
    { 1.0f, 1.0f, 0.0f },
    { 0.0f, 1.0f, 1.0f },
    { 1.0f, 0.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f }
};

const uint32_t indices[12][3] = {
    { 3, 5, 7 }, { 3, 7, 6 }, { 1, 6, 7 }, { 1, 7, 4 }, 
    { 0, 1, 4 }, { 0, 4, 2 }, { 0, 2, 5 }, { 0, 5, 3 },
    { 2, 5, 7 }, { 2, 7, 4 }, { 0, 1, 6 }, { 0, 6, 3 }
};

}  // anonymous namespace

void DirectVolume::initialize() {
    // Shader program
    program = std::make_shared<ShaderProgram>();
    program->create();
    program->addShaderFromFile("shaders/direct_volume.vert", ShaderType::Vertex);
    program->addShaderFromFile("shaders/direct_volume.frag", ShaderType::Fragment);
    program->link();

    // Empty VAO
    glGenVertexArrays(1, &vaoId);
    glBindVertexArray(vaoId);
    glBindVertexArray(0);
}

void DirectVolume::setLocation(const glm::mat4 &transform) {
    this->modelMat = transform;
}

void DirectVolume::draw(const Camera &camera, const PointLight &light, const std::unique_ptr<VolumeTexture> &volTex) {
    program->bind();
    {
        program->setUniformValue("u_mMat", modelMat);
        program->setUniformValue("u_mvpMat", camera.projMat * camera.viewMat * modelMat);
        program->setUniformValue("u_cameraPos", camera.pos);
        program->setUniformValue("u_sampleNum", volTex->maxExtentMargined());
        program->setUniformValue("u_albedo", volTex->albedo());

        const auto &corners = volTex->marginedCube().corners;
        program->setUniformValueArray("u_marginCubeVertices", corners.data(), corners.size());

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, volTex->getFilteredTexId());
        program->setUniformValue("u_filteredTex", 0);

        glBindVertexArray(vaoId);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }
    program->release();
}