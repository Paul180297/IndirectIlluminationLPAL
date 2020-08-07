#pragma once

#include <glm/glm.hpp>

struct PointLight {
    PointLight() {}
    PointLight(const glm::vec3 &pos, const glm::vec3 &Le)
        : pos{ pos }
        , Le{ Le } {
    }

    glm::vec3 Le = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);
};
