#pragma once

#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>

#include <glm/glm.hpp>

struct VolumeData {
	VolumeData();
	VolumeData(int sizeX, int sizeY, int sizeZ, int channels);
	VolumeData(const VolumeData &vol);
	~VolumeData();

	VolumeData &operator=(const VolumeData& vol);
	float &operator()(int x, int y, int z, int c);

	float *ptr() const { return data.get(); }

	void resize(int sizeX, int sizeY, int sizeZ, int channels);
	void setRange(float x_min, float y_min, float z_min, float x_max, float y_max, float z_max);
	void load(const std::string &filename);

	int totalSize() const {
    	return size.x * size.y * size.z * channels;
    }

	glm::ivec3 size = glm::ivec3(0, 0, 0);
	int channels = 0;
	glm::vec3 bboxMin;
	glm::vec3 bboxMax;
	std::unique_ptr<float[]> data = nullptr;
};
