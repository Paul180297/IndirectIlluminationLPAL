#pragma once

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <cassert>

static const float PI = 4.0f * std::atan(1.0f);

// -----------------------------------------------------------------------------
// Common header files
// -----------------------------------------------------------------------------

#define GLFW_INCLUDE_GLU
#define GLM_ENABLE_EXPERIMENTAL
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>

#include <stb_image.h>
#include <stb_image_write.h>
#include <tiny_obj_loader.h>

// -----------------------------------------------------------------------------
// Debug function
// -----------------------------------------------------------------------------

#define FatalError(...)               \
    do {                              \
        std::cerr << "[ERROR] ";      \
        fprintf(stderr, __VA_ARGS__); \
        std::cerr << std::endl;       \
        std::abort();                 \
    } while (false);

// -----------------------------------------------------------------------------
// Assertion with message
// -----------------------------------------------------------------------------

#ifndef __FUNCTION_NAME__
#if defined(_WIN32) || defined(__WIN32__)
#define __FUNCTION_NAME__ __FUNCTION__
#else
#define __FUNCTION_NAME__ __func__
#endif
#endif

#ifndef NDEBUG
#define Assertion(PREDICATE, ...) \
do { \
    if (!(PREDICATE)) { \
        std::cerr << "Asssertion \"" \
        << #PREDICATE << "\" failed in " << __FILE__ \
        << " line " << __LINE__ \
        << " in function \"" << (__FUNCTION_NAME__) << "\"" \
        << " : "; \
        fprintf(stderr, __VA_ARGS__); \
        std::cerr << std::endl; \
        std::abort(); \
    } \
} while (false)
#else  // NDEBUG
#define Assertion(PREDICATE, ...) do {} while (false)
#endif  // NDEBUG