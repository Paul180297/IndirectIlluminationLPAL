#pragma once

#include <memory>

#include "camera.h"
#include "point_light.h"
#include "vertex_array_object.h"
#include "shader_program.h"
#include "texture.h"

struct VolumeTexture;

class IndirectSurface { // : public RenderObject {
public:
    void initialize();
    void setMeshFromFile(const std::string &filename);
    void setRoughnessTexure(const std::string &filename);
    void draw(const Camera &camera, const PointLight &light, const std::unique_ptr<VolumeTexture> &volTex);

    void setRoughnessValue(float roughness) {
        this->roughness = roughness;
    }

    void setNumSections(int sections) {
        this->nSections = sections;
    }


private:
    GLuint ltcMatTexId;
    GLuint ltcMagTexId;

    float roughness = 0.001f;
    int nSections = 128;

    std::shared_ptr<VertexArrayObject> vao = nullptr;
    std::shared_ptr<ShaderProgram> program = nullptr;
    std::shared_ptr<Texture> roughnessTex = nullptr;
};
