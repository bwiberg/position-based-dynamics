#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace pbd {
    struct SceneSetup {
        static SceneSetup LoadFromJsonString(const std::string &str);

        std::string name;
        std::string filepath;

        struct {
            glm::vec3 position;
            float fovY;
        } camera;

        struct ShaderConfig {
            std::string name;
            std::string vertex;
            std::string fragment;
        };

        std::vector<ShaderConfig> shaders;

        struct MeshConfig {
            bool isCloth;
            std::string path;
            std::string shader;
            glm::vec3 position;
            glm::vec3 orientation;
            float scale;
        };

        std::vector<MeshConfig> meshes;
    };
}