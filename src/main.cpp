#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <memory>
#include <cstddef>
#include <iostream>
#include <algorithm>
#include <string>
#include <array>

#include "core/common.h"
#include "core/config.h"
#include "core/timer.h"
#include "core/point_light.h"
#include "core/direct_volume.h"
#include "core/indirect_surface.h"
#include "core/volume_texture.h"

static const int WIN_WIDTH = 960;
static const int WIN_HEIGHT = 960;
static const char *WIN_TITLE = "IndirectIlluminationLPAL";
static Config config;

// Texture size parameters
// |---|***********|---|
//  (2)     (1)     (2)
// (1) = INNTER_TEX_SIZE
// (2) = TEX_MARGIN
static constexpr int INNER_TEX_SIZE = 128;
static constexpr int TEX_MARGIN = 4;

Camera camera;
PointLight pointLight;

std::unique_ptr<DirectVolume> directVolume = nullptr;
std::unique_ptr<IndirectSurface> indirectSurface = nullptr;
std::unique_ptr<VolumeTexture> volTex = nullptr;
std::unique_ptr<ShaderProgram> gaussianCompShader = nullptr;
std::unique_ptr<ShaderProgram> radianceCompShader = nullptr;
std::unique_ptr<ShaderProgram> mipCompShader = nullptr;

static const std::array<glm::vec3, 8> REGULAR_CUBE = {
    glm::vec3(-1.0f, -1.0f, -1.0f),
    glm::vec3( 1.0f, -1.0f, -1.0f),
    glm::vec3(-1.0f,  1.0f, -1.0f),
    glm::vec3(-1.0f, -1.0f,  1.0f),
    glm::vec3( 1.0f,  1.0f, -1.0f),
    glm::vec3(-1.0f,  1.0f,  1.0f),
    glm::vec3( 1.0f, -1.0f,  1.0f),
    glm::vec3( 1.0f,  1.0f,  1.0f)
};

void saveCurrentBuffer(GLFWwindow *window, const std::string &filename) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    auto bytes = std::make_unique<uint8_t[]>(width * height * 4);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, (void *)bytes.get());

    // Invert vertically
    for (int y = 0; y < height / 2; y++) {
        for (int x = 0; x < width; x++) {
            const int iy = height - y - 1;
            for (int c = 0; c < 4; c++) {
                std::swap(bytes[(y * width + x) * 4 + c], bytes[(iy * width + x) * 4 + c]);
            }
        }
    }

    // Save
    stbi_write_png(filename.c_str(), width, height, 4, bytes.get(), 0);
    printf("Save: %s", filename.c_str());
}

// ----------------------------------------------------------------------------
// Scene state updates
// ----------------------------------------------------------------------------

void updateVolume() {
    volTex->updateVolume(pointLight.pos, pointLight.Le);
    volTex->gaussianFilter3D();
}

void updateCamera(int frame) {
    // Rotate camera position for demo program
    camera.pos.z = 19.0 * cos(0.5f * PI / 180.0f * frame - 0.5f * PI);
    camera.pos.x = 19.0 * sin(0.5f * PI / 180.0f * frame - 0.5f * PI);
    camera.pos.y = 6.5f + 3.0 * sin(1.0f * PI / 180.0f * (frame) - 0.5f * PI);
    camera.viewMat = glm::lookAt(camera.pos, camera.dir, camera.up);
}

// ----------------------------------------------------------------------------
// OpenGL and GLFW utilities
// ----------------------------------------------------------------------------

void initializeGL() {
    // OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Camera
    camera.pos = config.getVec3D("cameraPos");
    camera.dir = config.getVec3D("cameraDir");
    camera.up = config.getVec3D("cameraUp");
    camera.viewMat = glm::lookAt(camera.pos, camera.dir, camera.up);
    camera.projMat = glm::perspective(glm::radians(45.0f), float(WIN_WIDTH) / float(WIN_HEIGHT), 0.1f, 1000.0f);

    // Light
    pointLight.Le = config.getVec3D("lightLe");
    pointLight.pos = config.getVec3D("lightPos");

    // Scene
    indirectSurface = std::make_unique<IndirectSurface>();
    indirectSurface->initialize();
    indirectSurface->setMeshFromFile(config.getPath("meshFile"));
    indirectSurface->setNumSections(config.getInt("numSlices"));
    indirectSurface->setRoughnessTexure(config.getPath("roughTexFile"));

    // Calculate volume transformation
    static const glm::mat4 volScale = glm::scale(glm::vec3(3.3f));
    static const glm::mat4 volMarginScale = glm::scale(glm::vec3((float)(INNER_TEX_SIZE + 2 * TEX_MARGIN) / (float)INNER_TEX_SIZE)) * volScale;
    static const glm::mat4 volRotate = glm::rotate(0.0f * PI, glm::vec3(0.0f, 1.0f, 0.0f)) *
                                       glm::rotate(-0.0f * PI, glm::vec3(0.0f, 0.1f, 1.0f));
    static const glm::mat4 volTranslate = glm::translate(glm::vec3(0.0f, 4.0f, 0.0f));

    // Calculate bounding box for volume
    std::array<glm::vec3, 8> innerCube;
    std::array<glm::vec3, 8> marginedCube;
    for (int i = 0; i < 8; i++) {
        // Internal cube without margin
        glm::vec4 vc = volTranslate * volRotate * volScale * glm::vec4(REGULAR_CUBE[i], 1.0f);
        innerCube[i] = glm::vec3(vc) / vc.w;

        // External cube with margin
        glm::vec4 mc = volTranslate * volRotate * volMarginScale * glm::vec4(REGULAR_CUBE[i], 1.0f);
        marginedCube[i] = glm::vec3(mc) / mc.w;
    }

    // Load volume and set rendering parameters
    const glm::ivec3 innerTexSize(INNER_TEX_SIZE, INNER_TEX_SIZE, INNER_TEX_SIZE);
    const glm::ivec3 marginSize(TEX_MARGIN, TEX_MARGIN, TEX_MARGIN);

    volTex = std::make_unique<VolumeTexture>(innerTexSize, marginSize, innerCube, marginedCube);
    volTex->initialize();
    volTex->setAlbedo(config.getVec3D("albedo"));
    volTex->setEmission(config.getVec3D("emission"));
    volTex->setDensityScale(config.getFloat("densityScale"));
    volTex->setVolumeType(VolumeType::Emissive);
    volTex->readVolumeData(config.getPath("volumeFolder"), "density", "emission");

    // volume (for ray marching)
    directVolume = std::make_unique<DirectVolume>();
    directVolume->initialize();
    directVolume->setLocation(volTranslate * volRotate * volMarginScale);

    // preparation for drawing the first frame
    updateVolume();
}

void paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // indirectSurface
    indirectSurface->draw(camera, pointLight, volTex);

    // directly visible volume (ray marching)
    directVolume->draw(camera, pointLight, volTex);

    if (volTex->numFrames() > 1) {
        updateVolume();
    }
}

void resize(GLFWwindow* window, int width, int height) {
    // Update window size
    glfwSetWindowSize(window, width, height);

    // Update viewport following actual window size
    int renderBufferWidth, renderBufferHeight;
    glfwGetFramebufferSize(window, &renderBufferWidth, &renderBufferHeight);
    glViewport(0, 0, renderBufferWidth, renderBufferHeight);

    // Update camera
    camera.projMat = glm::perspective(glm::radians(45.0f), float(width) / float(height), 0.1f, 1000.0f);
}

void keyboard(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, true);
        }

        if (key == GLFW_KEY_S && mods == GLFW_MOD_CONTROL) {
            saveCurrentBuffer(window, "output.png");
        }
    }
}

// ----------------------------------------------------------------------------
// Main
// ----------------------------------------------------------------------------

int main(int argc, char **argv) {
    if (argc <= 1) {
        fprintf(stderr, "[ USAGE ] main.exe [ path to config.txt ]\n");
        std::exit(1);
    }

    // Load config
    config.load(argv[1]);

    // Setup GLFW
    if (glfwInit() == GL_FALSE) {
        fprintf(stderr, "Initialization failed!\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, WIN_TITLE, NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Window creation failed!\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (gladLoadGL() != GL_TRUE) {
        FatalError("GLAD: failed to load OpenGL library!");
    }

    // Initialize general OpenGL functinalities
    initializeGL();

    // Set callback functions
    glfwSetWindowSizeCallback(window, resize);
    glfwSetKeyCallback(window, keyboard);

    // Timer for indicating fps
    GLtimer timer;
    timer.start();

    uint32_t frames = 0u;
    const uint32_t fpsInterval = 30u;

    // Mainloop
    while (glfwWindowShouldClose(window) == GL_FALSE) {
        // Draw
        timer.start();
        {
            paintGL();
        }
        timer.end();

        // Update scene params
        updateCamera(frames);

        // Display time and fps
        if (frames % fpsInterval == 0) {
            char title[256];
            const double duration = timer.getDuration(fpsInterval);
            sprintf(title, "%s: %.2f [fps], %.3f [ms/frame]", WIN_TITLE, 1000.0 / duration, duration);
            glfwSetWindowTitle(window, title);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
        frames++;
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
