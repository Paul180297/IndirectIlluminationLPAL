#pragma once

#include <glm/glm.hpp>

struct Camera {
	glm::vec3 pos;
	glm::vec3 dir;
	glm::vec3 up;
	glm::mat4 projMat;
	glm::mat4 viewMat;
};
