#include "indirect_surface.h"

#include <ctime>
#include <iostream>
#include <deque>

#include "common.h"
#include "ltc_texture.h"
#include "volume_texture.h"

void IndirectSurface::initialize() {
    ltcMatTexId = createLTCmatTex();
    ltcMagTexId = createLTCmagTex();

    // Shader program
    program = std::make_shared<ShaderProgram>();
    program->create();
    program->addShaderFromFile("shaders/indirect_LPAL.vert", ShaderType::Vertex);
    program->addShaderFromFile("shaders/indirect_LPAL.frag", ShaderType::Fragment);
    program->link();

    // Vertex array object
    vao = std::make_shared<VertexArrayObject>();
    vao->create();
}

void IndirectSurface::setMeshFromFile(const std::string& filename) {
    vao->addMeshFromFile(filename);
}

void IndirectSurface::setRoughnessTexure(const std::string &filename) {
    roughnessTex = std::make_shared<Texture>(filename, true);
}

void IndirectSurface::draw(const Camera &camera, const PointLight &light, const std::unique_ptr<VolumeTexture> &volTex) {
    program->bind();
    {
        glm::mat4 mMat, mvMat, mvpMat, normMat;
        mMat = glm::mat4(1.0f);
        mvMat = camera.viewMat * mMat;
        mvpMat = camera.projMat * mvMat;
        normMat = glm::transpose(glm::inverse(mvMat));

        program->setUniformValue("u_lightMat", camera.viewMat);
        program->setUniformValue("u_mMat", mMat);
        program->setUniformValue("u_mvMat", mvMat);
        program->setUniformValue("u_mvpMat", mvpMat);
        program->setUniformValue("u_normMat", normMat);

        program->setUniformValue("u_cameraPos", camera.pos);
        program->setUniformValue("u_lightPos", light.pos);
        program->setUniformValue("u_lightLe", light.Le);
        program->setUniformValue("u_sectionNum", nSections);
        program->setUniformValue("u_alpha", roughness);
        program->setUniformValue("u_isAlphaTextured", roughnessTex != nullptr ? 1 : 1);
        program->setUniformValue("u_albedo", volTex->albedo());
        program->setUniformValue("u_maxLOD", volTex->maxLod());

        const auto &marginCube = volTex->marginedCube();
        program->setUniformValueArray("u_marginCubeVertices", marginCube.corners.data(), marginCube.corners.size());
        program->setUniformValue("u_cubeCenter", marginCube.center);

        const auto &innerCube = volTex->innerCube();
        program->setUniformValueArray("u_originalCubeVertices", innerCube.corners.data(), innerCube.corners.size());

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ltcMatTexId);
        program->setUniformValue("u_ltcMatTex", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, ltcMagTexId);
        program->setUniformValue("u_ltcMagTex", 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, roughnessTex->getId());
        program->setUniformValue("u_alphaTex", 2);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_3D, volTex->getFilteredTexId());
        program->setUniformValue("u_filteredTex", 3);

        vao->draw(GL_TRIANGLES);
    }
    program->release();
}