#pragma once

#include "Light.hpp"
#include "Attenuation.hpp"
#include "rendering/SceneObject.hpp"

namespace clgl {
    /// @brief //todo add brief description to DirectionalLight
    /// @author Benjamin Wiberg
    class DirectionalLight : public SceneObject, public Light {
    public:
        DirectionalLight(const glm::vec3 &ambientColor = Light::DefaultColor,
                         const glm::vec3 &diffuseColor = Light::DefaultColor,
                         const glm::vec3 &specularColor = Light::DefaultColor,
                         const glm::vec3 &lightDirection = glm::vec3(0.0f, -1.0f, 0.0f));

        virtual void setUniformsInShader(std::shared_ptr<BaseShader> shader,
                                         const std::string &prefix = "") override;

        glm::vec3 mLightDirection;
    };
}
