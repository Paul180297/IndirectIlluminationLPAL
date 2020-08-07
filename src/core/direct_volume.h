#pragma once

#include <array>
#include <memory>

#include "common.h"
#include "camera.h"
#include "point_light.h"
#include "shader_program.h"

struct VolumeTexture;

class DirectVolume {
public:
	void initialize();
	void setLocation(const glm::mat4 &transform);
	void draw(const Camera &camera, const PointLight &light, const std::unique_ptr<VolumeTexture> &volTex);

private:
    GLuint vaoId;
    glm::mat4 modelMat = glm::mat4(1.0f);
    std::shared_ptr<ShaderProgram> program = nullptr;
};
