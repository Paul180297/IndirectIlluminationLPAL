#include "volume_data.h"

#include <cstring>
#include <algorithm>

static int nextPowerOfTwo(int v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;

	return v;
}

VolumeData::VolumeData() {
	const float vmin = std::numeric_limits<float>::min();
	const float vmax = std::numeric_limits<float>::max();
	bboxMin = glm::vec3(vmax, vmax, vmax);
	bboxMax = glm::vec3(vmin, vmin, vmin);
}

VolumeData::VolumeData(int sizeX, int sizeY, int sizeZ, int channels)
	: VolumeData() {
	size = glm::ivec3(sizeX, sizeY, sizeZ);
	this->channels = channels;
	data = std::make_unique<float[]>(size.x * size.y * size.z * channels);
	std::memset(data.get(), 0, sizeof(float) * size.x * size.y * size.z * channels);
}

VolumeData::VolumeData(const VolumeData &vol)
	: VolumeData() {
	this->operator=(vol);
}

VolumeData::~VolumeData() {
}

VolumeData &VolumeData::operator=(const VolumeData &vol) {
	size = vol.size;
	channels = vol.channels;
	bboxMin = vol.bboxMin;
	bboxMax = vol.bboxMax;

	data = std::make_unique<float[]>(size.x * size.y * size.z * channels);
	std::memcpy(data.get(), vol.data.get(),
		sizeof(float) * size.x * size.y * size.z * channels);

    return *this;
}

float &VolumeData::operator()(int x, int y, int z, int c) {
	return data[((z * size.y + y) * size.x + x) * channels + c];
}

void VolumeData::resize(int sizeX, int sizeY, int sizeZ, int channels) {
	size = glm::ivec3(sizeX, sizeY, sizeZ);
	this->channels = channels;
	data = std::make_unique<float[]>(size.x * size.y * size.z * channels);
	std::memset(data.get(), 0, sizeof(float) * size.x * size.y * size.z * channels);
}

void VolumeData::setRange(float x_min, float y_min, float z_min, float x_max, float y_max, float z_max) {
	bboxMin = glm::vec3(x_min, y_min, z_min);
	bboxMax = glm::vec3(x_max, y_max, z_max);
}

void VolumeData::load(const std::string &filename) {
	std::ifstream ifs(filename.c_str(), std::ios::binary);
	if (ifs.fail()) {
		fprintf(stderr, "unable to open file: %s\n", filename.c_str());
		exit(1);
	}

	char identifier[4] = { 0 };
	ifs.read((char*)identifier, sizeof(char) * 3);

	if (strcmp(identifier, "VOL") != 0) { // returns 1, -1, 0
		fprintf(stderr, "invalid identifier: \"%s\"\n", identifier);
		exit(1);
	}

	char version;
	ifs.read((char*)&version, sizeof(char));
	if (version != 3) {
		fprintf(stderr, "invalid version number: %d\n", (int)version);
		exit(1);
	}

	int type;
	ifs.read((char*)&type, sizeof(int));
	if (type != 1) {
		fprintf(stderr, "only float32 supported (type = %d, this should be 1)\n", (int)type);
		exit(1);
	}

	int orgSizeX, orgSizeY, orgSizeZ;
	ifs.read((char*)&orgSizeX, sizeof(int) * 1);
	ifs.read((char*)&orgSizeY, sizeof(int) * 1);
	ifs.read((char*)&orgSizeZ, sizeof(int) * 1);
	ifs.read((char*)&channels, sizeof(int) * 1);

	ifs.read((char*)&bboxMin.x, sizeof(float) * 1);
	ifs.read((char*)&bboxMin.y, sizeof(float) * 1);
	ifs.read((char*)&bboxMin.z, sizeof(float) * 1);
	ifs.read((char*)&bboxMax.x, sizeof(float) * 1);
	ifs.read((char*)&bboxMax.y, sizeof(float) * 1);
	ifs.read((char*)&bboxMax.z, sizeof(float) * 1);

	// Roll up to the size to its next power of two.
	size.x = nextPowerOfTwo(orgSizeX);
	size.y = nextPowerOfTwo(orgSizeY);
	size.z = nextPowerOfTwo(orgSizeZ);

	// Resize bounding box.
	const float dx = (bboxMax.x - bboxMin.x) / orgSizeX;
	const float dy = (bboxMax.y - bboxMin.y) / orgSizeY;
	const float dz = (bboxMax.z - bboxMin.z) / orgSizeZ;
	bboxMax.x = bboxMax.x + (size.x - orgSizeX) * dx;
	bboxMax.y = bboxMax.y + (size.y - orgSizeY) * dy;
	bboxMax.z = bboxMax.z + (size.z - orgSizeZ) * dz;

	// Load volume data
	int maxExtent = std::max(size.x, std::max(size.y, size.z));
	float *buffer = new float[maxExtent * channels];
	data = std::make_unique<float[]>(size.x * size.y * size.z * channels);
	for (int z = 0; z < orgSizeZ; z++) {
		for (int y = 0; y < orgSizeY; y++) {
			ifs.read((char*)buffer, sizeof(float) * orgSizeX * channels);			
            for (int x = 0; x < orgSizeX; x++) {
				for (int c = 0; c < channels; c++) {
					(*this)(x, y, z, c) = buffer[x * channels + c];
				}
			}
		}
	}
}
