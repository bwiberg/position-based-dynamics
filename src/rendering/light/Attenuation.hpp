#pragma once

namespace clgl {
    struct Attenuation {
        inline Attenuation(float linear_ = 0.0f, float quadratic_ = 0.0f)
                : linear(linear_), quadratic(quadratic_) {}

        float linear;
        float quadratic;
    };
}