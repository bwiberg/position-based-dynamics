#pragma once

#include "rendering/BaseShader.hpp"

namespace clgl {
    namespace LightType {
        const std::string DIRECTIONAL = "directional";
        const std::string POINT = "point";
    }

    class Light {
    public:
        const static glm::vec3 DefaultColor;

        Light(const std::string &type,
                     const glm::vec3 &ambientColor,
                     const glm::vec3 &diffuseColor,
                     const glm::vec3 &specularColor);

        void setUniformsInShader(std::shared_ptr<BaseShader> shader);

        virtual void setUniformsInShader(std::shared_ptr<BaseShader> shader,
                                         const std::string &prefix) = 0;

        glm::vec3 mAmbientColor;
        glm::vec3 mDiffuseColor;
        glm::vec3 mSpecularColor;

        const std::string mType;
    };
}