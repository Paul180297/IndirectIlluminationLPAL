#include <iostream>
#include <fstream>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

#include "common.h"
#include "volume_texture.h"
#include "volume_data.h"

static constexpr double eps = 1.0e-8;
static const double pi = 4.0 * std::atan(1.0);

VolumeTexture::VolumeTexture(const glm::ivec3 &innerTexSize, const glm::ivec3 &marginSize, const std::array<glm::vec3, 8> &innerCube, const std::array<glm::vec3, 8> &marginCube)
    : innerTexSize_{ innerTexSize }
    , marginSize_{ marginSize }
    , innerCube_{ innerCube }
    , marginedCube_{ marginCube } {    
}

VolumeTexture::~VolumeTexture() {
}

void VolumeTexture::initialize() {
    // Build compute shader program    
    injectRadianceProgram = std::make_shared<ShaderProgram>();
    injectRadianceProgram->create();
    injectRadianceProgram->addShaderFromFile("shaders/calcRadiance.comp", ShaderType::Compute);
    injectRadianceProgram->link();
    
    mipmapProgram = std::make_shared<ShaderProgram>();
    mipmapProgram->create();
    mipmapProgram->addShaderFromFile("shaders/mipmap.comp", ShaderType::Compute);
    mipmapProgram->link();

    gaussFilterProgram = std::make_shared<ShaderProgram>();
    gaussFilterProgram->create();
    gaussFilterProgram->addShaderFromFile("shaders/gaussianFilter.comp", ShaderType::Compute);
    gaussFilterProgram->link();

    // Allocate 3D textures
    const glm::ivec3 extSize = marginedTexSize();
    const int mipLevels = maxLod();
    {
        // Radiant intensity (which is computed internally with ray marching)
        glGenTextures(1, &radDensTexId);
        glBindTexture(GL_TEXTURE_3D, radDensTexId);
        glTexStorage3D(GL_TEXTURE_3D, mipLevels, GL_RGBA32F, extSize.x, extSize.y, extSize.z);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

        glBindTexture(GL_TEXTURE_3D, 0);
    }

    if (type == VolumeType::Emissive) {
        // Emission (which is externally provided by a .vol file)
        // This is optional for emissive volumes
        glGenTextures(1, &emissionTexId);
        glBindTexture(GL_TEXTURE_3D, emissionTexId);
        glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA32F, extSize.x, extSize.y, extSize.z);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

        glBindTexture(GL_TEXTURE_3D, 0);
    }

    {
        // Density (which is externally provided by a .vol file)
        glGenTextures(1, &densityTexId);
        glBindTexture(GL_TEXTURE_3D, densityTexId);
        glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32F, extSize.x, extSize.y, extSize.z);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

        glBindTexture(GL_TEXTURE_3D, 0);
    }

    // Filter destination
    {
        glGenTextures(1, &filteredTexId);
        glBindTexture(GL_TEXTURE_3D, filteredTexId);
        glTexStorage3D(GL_TEXTURE_3D, mipLevels, GL_RGBA32F, extSize.x, extSize.y, extSize.z);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

        glBindTexture(GL_TEXTURE_3D, 0);
    }

    // Buffer for Gaussian filter
    {
        glGenTextures(1, &filterBufferId);
        glBindTexture(GL_TEXTURE_3D, filterBufferId);
        glTexStorage3D(GL_TEXTURE_3D, mipLevels, GL_RGBA32F, extSize.x, extSize.y, extSize.z);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

        glBindTexture(GL_TEXTURE_3D, 0);
    }


    // Precompute Gaussian filter kenel
    {
        // Calculate kernel coefficients
        const float beta = 1.0f / (2.0f * sigmaGauss * sigmaGauss);
        const int size = (int)(sigmaGauss * 2.0f);
        std::vector<float> kernel(size * 2 + 1);
    
        float sumWgt = 0.0f;
        for (int k = -size; k <= size; k++) {
            const float weight = std::exp(-beta * k * k);
            kernel[k + size] = weight;
            sumWgt += weight;
        }

        for (int k = -size; k <= size; k++) {
            kernel[k + size] /= sumWgt;
        }

        // Setup texture buffer
        kernelTexBuffer = std::make_shared<TextureBuffer>(kernel.size() * sizeof(float), GL_R32F, GL_STATIC_DRAW);
        kernelTexBuffer->setData(kernel.data());
    }
}

void VolumeTexture::destroy() {
    if (densityTexId != 0) {
        glDeleteTextures(1, &densityTexId);
        densityTexId = 0;
    }

    if (emissionTexId != 0) {
        glDeleteTextures(1, &emissionTexId);
        emissionTexId = 0;
    }

    if (radDensTexId != 0) {
        glDeleteTextures(1, &radDensTexId);
        radDensTexId = 0;
    }

    if (filteredTexId != 0) {
        glDeleteTextures(1, &filteredTexId);
        filteredTexId = 0;
    }

    if (filterBufferId != 0) {
        glDeleteTextures(1, &filterBufferId);
        filterBufferId = 0;
    }
}

void VolumeTexture::readVolumeData(const std::string &folder, const std::string &densityPrefix, const std::string &emissionPrefix) {
    const bool hasEmission = emissionPrefix != "";

    std::vector<std::string> densityFiles;
    std::vector<std::string> emissionFiles;
    fs::path dirpath(folder.c_str());
    for (const auto& f : fs::directory_iterator(dirpath)) {
        if (f.path().string().find(densityPrefix) != std::string::npos) {
            densityFiles.push_back(f.path().string());
        }

        if (hasEmission && f.path().string().find(emissionPrefix) != std::string::npos) {
            emissionFiles.push_back(f.path().string());
        }
    }
    
    numFrames_ = densityFiles.size();
    if (hasEmission && numFrames_ != emissionFiles.size()) {
        FatalError("# of density and emission volumes are different!");
    }

    densityDataArray.resize(numFrames_);
    if (hasEmission) {
        emissionDataArray.resize(numFrames_);
    }

    const glm::ivec3 extSize = marginedTexSize();
    for (int i = 0; i < numFrames_; i++) {
        VolumeData densityData, emissionData;
        densityData.load(densityFiles[i]);
        if (hasEmission) {
            emissionData.load(emissionFiles[i]);
        }

        densityDataArray[i].resize(extSize.x, extSize.y, extSize.z, 1);
        emissionDataArray[i].resize(extSize.x, extSize.y, extSize.z, 3);

        const glm::ivec3 extSize = marginedTexSize();
        float *densityPtr = densityData.ptr();
        float *emissionPtr = nullptr;
        if (hasEmission) {
            emissionPtr = emissionData.ptr();
        }

        for (int z = 0; z < extSize.z; z++) {
            for (int y = 0; y < extSize.y; y++) {
                for (int x = 0; x < extSize.x; x++) {
                    const int index = (z * extSize.y + y) * extSize.x + x;
                    if ((x < marginSize_.x || x >= innerTexSize_.x + marginSize_.x) || 
                        (y < marginSize_.y || y >= innerTexSize_.y + marginSize_.y) ||
                        (z < marginSize_.z || z >= innerTexSize_.z + marginSize_.z)) {
                        densityDataArray[i](x, y, z, 0) = 0.0f;
                        emissionDataArray[i](x, y, z, 0) = 0.0f;
                        emissionDataArray[i](x, y, z, 1) = 0.0f;
                        emissionDataArray[i](x, y, z, 2) = 0.0f;
                    } else {
                        densityDataArray[i](x, y, z, 0) = (*densityPtr) * densityScale_;
                        ++densityPtr;

                        if (hasEmission) {
                            emissionDataArray[i](x, y, z, 0) = *emissionPtr;
                            emissionDataArray[i](x, y, z, 1) = *emissionPtr;
                            emissionDataArray[i](x, y, z, 2) = *emissionPtr;
                            ++emissionPtr;
                        } else {
                            emissionDataArray[i](x, y, z, 0) = 0.0f;
                            emissionDataArray[i](x, y, z, 1) = 0.0f;
                            emissionDataArray[i](x, y, z, 2) = 0.0f;
                        }
                    }
                }
            }
        }

        printf("\r[ %d / %d ] volumes loaded...", i + 1, numFrames_);
    }
    printf("\nOK!\n");

    // Reset frame
    frame = 0;
}

void VolumeTexture::updateVolume(const glm::vec3 &lightPos, const glm::vec3 &lightLe) {
    const glm::ivec3 extSize = marginedTexSize();

    glBindTexture(GL_TEXTURE_3D, densityTexId);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, extSize.x, extSize.y, extSize.z, GL_RED, GL_FLOAT, densityDataArray[frame].ptr());
    glBindTexture(GL_TEXTURE_3D, 0);

    glBindTexture(GL_TEXTURE_3D, emissionTexId);    
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, extSize.x, extSize.y, extSize.z, GL_RGB, GL_FLOAT, emissionDataArray[frame].ptr());
    glBindTexture(GL_TEXTURE_3D, 0);

    static const int localSize = 4;

    // Calculate incident radiant intensity to each voxel
    injectRadianceProgram->bind();
    {
        glBindImageTexture(0, densityTexId, 0, GL_TRUE, 0, GL_READ_ONLY, GL_R32F);
        glBindImageTexture(1, radDensTexId, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

        if (type == VolumeType::Emissive) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_3D, emissionTexId);
            injectRadianceProgram->setUniformValue("u_emissionTex", 2);
        }

        injectRadianceProgram->setUniformValue("u_type", (int)type);
        injectRadianceProgram->setUniformValue("u_lightLe", lightLe);
        injectRadianceProgram->setUniformValue("u_lightPos", lightPos);
        injectRadianceProgram->setUniformValue("u_emissionColor", emission_);
        injectRadianceProgram->setUniformValue("u_albedo", albedo_);
        injectRadianceProgram->setUniformValueArray("u_cubeCorners", marginedCube_.corners.data(), marginedCube_.corners.size());

        const glm::ivec3 extSize = marginedTexSize();
        injectRadianceProgram->setUniformValue("u_marginTexSize", extSize);

        const int numGroupSizeX = (extSize.x + localSize - 1) / localSize;  
        const int numGroupSizeY = (extSize.y + localSize - 1) / localSize; 
        const int numGroupSizeZ = (extSize.z + localSize - 1) / localSize;  

        glDispatchCompute(numGroupSizeX, numGroupSizeY, numGroupSizeZ);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
    injectRadianceProgram->release();

    // GPU based MIP mapping
    mipmapProgram->bind();
    {
        const int mipLevels = maxLod();
        glm::ivec3 levelSize = marginedTexSize();
        for (int level = 1; level < mipLevels; level++) {
            glBindImageTexture(0, radDensTexId, level - 1, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
            glBindImageTexture(1, radDensTexId, level, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

            levelSize /= 2;
            const int numGroupSizeX = (levelSize.x + localSize - 1) / localSize;  
            const int numGroupSizeY = (levelSize.y + localSize - 1) / localSize; 
            const int numGroupSizeZ = (levelSize.z + localSize - 1) / localSize;  
            
            glDispatchCompute(numGroupSizeX, numGroupSizeY, numGroupSizeZ);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }
    }
    mipmapProgram->release();

    // Increment frame
    frame = (frame + 1) % numFrames_;
}

void VolumeTexture::gaussianFilter3D() {
    static const int localSize = 4;

    gaussFilterProgram->bind();
    {
        const int mipLevels = maxLod();
        std::vector<glm::ivec3> texSizeLod(mipLevels + 1);

        const glm::vec3 marginedSize = marginedTexSize();
        texSizeLod[0] = marginedTexSize();
        for (int level = 1; level < mipLevels + 1; level++) {
            texSizeLod[level] = texSizeLod[level - 1] / 2;
        }
        
        for (int level = mipLevels; level >= 0; level--) {
            glBindImageTexture(0, radDensTexId, level, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
            glBindImageTexture(1, filteredTexId, level, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
            glBindImageTexture(2, filterBufferId, level, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);

            gaussFilterProgram->setUniformValue("u_lodTexSize", texSizeLod[level]);
            gaussFilterProgram->setUniformValue("u_marginSize", marginSize_);
            gaussFilterProgram->setUniformValue("u_LOD", level);
            gaussFilterProgram->setUniformValue("u_maxLOD", mipLevels);

            gaussFilterProgram->setUniformValue("u_kernelSize", (int)(kernelTexBuffer->size() / sizeof(float)));
            kernelTexBuffer->bind(0);
            gaussFilterProgram->setUniformValue("u_gaussKernel", 0);

            const int numGruopSizeX = (texSizeLod[level].x + localSize - 1) / localSize;
            const int numGruopSizeY = (texSizeLod[level].y + localSize - 1) / localSize;
            const int numGruopSizeZ = (texSizeLod[level].z + localSize - 1) / localSize;

            glDispatchCompute(numGruopSizeX, numGruopSizeY, numGruopSizeZ);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }
    }
    gaussFilterProgram->release();
}
