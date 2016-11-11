#pragma once

#include <string>
#include <nanogui/opengl.h>

namespace pbd {
    struct Texture {
        GLuint ID;
        std::string type;
        std::string path;
    };
}