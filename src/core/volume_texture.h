#pragma once

#include <array>
#include <vector>

#include "common.h"
#include "shader_program.h"
#include "texture_buffer.h"
#include "volume_data.h"

enum class VolumeType : uint32_t {
    NonEmissive = 0,
    Emissive = 1
};

struct Cube {
    Cube() {}
    Cube(const std::array<glm::vec3, 8>& corners)
        : corners{ corners } {
        center = glm::vec3(0.0f);
        for (const auto& v : corners) {
            center += v;
        }
        center /= corners.size();
    }

    std::array<glm::vec3, 8> corners;
    glm::vec3 center;
};

class VolumeTexture {
public:
    VolumeTexture(const glm::ivec3 &innerTexSize, const glm::ivec3 &marginSize, const std::array<glm::vec3, 8> &innerCube, const std::array<glm::vec3, 8> &marginedCube);
    virtual ~VolumeTexture();
    void destroy();

    VolumeTexture(const VolumeTexture &) = delete;
    VolumeTexture &operator=(const VolumeTexture &) = delete;

    void initialize();
    void readVolumeData(const std::string &folder, const std::string &densityPrefix, const std::string &emissionPrefix = "");
    void updateVolume(const glm::vec3 &lightPos, const glm::vec3 &lightLe);
    void gaussianFilter3D();

    int totalSizeInner() const {
        return innerTexSize_.x * innerTexSize_.y * innerTexSize_.z;
    }

    int totalSizeMargined() const {
        const glm::ivec3 size = marginedTexSize();
        return size.x * size.y * size.z;
    }

    int maxExtent() const {
        return std::max(innerTexSize_.x, std::max(innerTexSize_.y, innerTexSize_.z));
    }

    int maxExtentMargined() const {
        const glm::vec3 size = marginedTexSize();
        return std::max(size.x, std::max(size.y, size.z));
    }

    glm::vec3 albedo() const {
        return albedo_;
    }

    void setAlbedo(const glm::vec3& albedo) {
        this->albedo_ = albedo;
    }

    glm::vec3 emission() const {
        return emission_;
    }

    void setEmission(const glm::vec3& emission) {
        this->emission_ = emission;
    }

    float densityScale() const {
        return densityScale_;
    }

    void setDensityScale(float scale) {
        this->densityScale_ = scale;
    }

    void setVolumeType(VolumeType type) {
        this->type = type;
    }

    glm::ivec3 innerTexSize() const {
        return innerTexSize_;
    }

    glm::ivec3 marginSize() const {
        return marginSize_;
    }

    glm::ivec3 marginedTexSize() const {
        return innerTexSize_ + 2 * marginSize_;
    }

    const int maxLod() const {
        const glm::vec3 size = marginedTexSize();
        return (int)std::floor(std::log2(std::max(size.x, std::max(size.y, size.z))));
    }

    const Cube& innerCube() const {
        return innerCube_;
    }

    const Cube& marginedCube() const {
        return marginedCube_;
    }

    GLuint getFilteredTexId() const {
        return filteredTexId;
    }

    const int numFrames() const {
        return numFrames_;
    }

private:
    glm::ivec3 innerTexSize_;
    glm::ivec3 marginSize_;

    glm::vec3 albedo_ = glm::vec3(0.3f);
    glm::vec3 emission_ = glm::vec3(1.0f);
    float densityScale_ = 1.0f;
    VolumeType type = VolumeType::Emissive;

    Cube innerCube_, marginedCube_;

    const float sigmaGauss = 2.0f;
    std::shared_ptr<TextureBuffer> kernelTexBuffer = nullptr;

    GLuint densityTexId = 0;    // Volume density
    GLuint emissionTexId = 0;   // Volume emission (optional)
    GLuint radDensTexId = 0;    // Incident radiant intensity
    GLuint filteredTexId = 0;   // Filtered radiant intensity and volume density (RGB: rad, A: density)
    GLuint filterBufferId = 0;  // Buffer for Gaussian filter

    std::shared_ptr<ShaderProgram> injectRadianceProgram = nullptr;
    std::shared_ptr<ShaderProgram> mipmapProgram = nullptr;
    std::shared_ptr<ShaderProgram> gaussFilterProgram = nullptr;

    int numFrames_ = 1;
    int frame = 0;
    std::vector<VolumeData> densityDataArray;
    std::vector<VolumeData> emissionDataArray;
};
