#pragma once

#include <vector>
#include <random>

#include "glm/glm.hpp"

namespace util {
    template<typename T>
    inline const T &min(const T &A, const T &B, const T &C) {
        return std::min(A, std::min(B, C));
    }

    template<typename T>
    inline const T &max(const T &A, const T &B, const T &C) {
        return std::max(A, std::max(B, C));
    }

    template<typename T>
    inline const T &median(const T &A, const T &B, const T &C) {
        return std::max(std::min(A, B), std::min(std::max(A, B), C));
    }


    template<typename T>
    inline const T &clamp(const T &value, const T &min, const T &max) {
        return std::max(min, std::min(max, value));
    }


    extern std::random_device random_device;

    /// Generates a vector of floats uniformly distributed in the specified range
    /// The vector of random floats
    std::vector<float> generate_uniform_floats(unsigned int N,
                                               float lower_bound_inclusive = 0.0f,
                                               float upper_bound_exclusive = 1.0f);

    /// Generates a vector of glm::vec3's with each x/y/z uniformly distributed in the specified range
    /// The vector of random glm::vec3's
    std::vector<glm::vec3> generate_uniform_vec3s(unsigned int N,
                                                  float x_lower_bound_inclusive = 0.0f,
                                                  float x_upper_bound_exclusive = 1.0f,
                                                  float y_lower_bound_inclusive = 0.0f,
                                                  float y_upper_bound_eyclusive = 1.0f,
                                                  float z_lower_bound_inclusive = 0.0f,
                                                  float z_upper_bound_ezclusive = 1.0f);

    std::vector<float> generate_linear_floats(unsigned int N,
                                              float lower_bound_inclusive = 0.0f,
                                              float upper_bound_inclusive = 1.0f);

    std::vector<glm::vec3> generate_linear_vec3s(unsigned int N,
                                                 float x_lower_bound_inclusive = 0.0f,
                                                 float x_upper_bound_inclusive = 1.0f,
                                                 float y_lower_bound_inclusive = 0.0f,
                                                 float y_upper_bound_inclusive = 1.0f,
                                                 float z_lower_bound_inclusive = 0.0f,
                                                 float z_upper_bound_inclusive = 1.0f);
}
