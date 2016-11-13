#pragma once

#include <string>
#include <nanogui/opengl.h>

namespace pbd {
    struct Texture {
        enum Type {
            DIFFUSE = 0,
            SPECULAR,
            BUMP
        };

        GLuint ID;
        Type type;
        std::string path;
    };
}