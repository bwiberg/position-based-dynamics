#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace pbd {
    struct CameraConfig {
        glm::vec3 position;
        float fovY;
    };

    struct ShaderConfig {
        std::string name;
        std::string vertex;
        std::string fragment;
    };

    struct ColorConfig {
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
    };

    struct AttenuationConfig {
        float linear;
        float quadratic;
    };

    struct DirectionalLightConfig {
        glm::vec3 direction;
        ColorConfig color;
    };

    struct PointLightConfig {
        glm::vec3 position;
        ColorConfig color;
        AttenuationConfig attenuation;
    };

    struct MeshConfig {
        bool isCloth;
        std::string path;
        std::string shader;
        glm::vec3 position;
        glm::vec3 orientation;
        float scale;
        bool flipNormals;
    };

    struct SceneSetup {
        static SceneSetup LoadFromJsonString(const std::string &str);

        std::string name;
        std::string filepath;

        CameraConfig camera;

        std::vector<DirectionalLightConfig> directionalLights;
        std::vector<PointLightConfig> pointLights;

        std::vector<ShaderConfig> shaders;
        std::vector<MeshConfig> meshes;
    };
}