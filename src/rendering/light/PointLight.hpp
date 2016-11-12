#pragma once

#include "Light.hpp"
#include "Attenuation.hpp"
#include "rendering/SceneObject.hpp"

namespace clgl {
    /// @brief //todo add brief description to PointLight
    /// @author Benjamin Wiberg
    class PointLight : public Light, public SceneObject {
    public:
        PointLight(const glm::vec3 &ambientColor = Light::DefaultColor,
                   const glm::vec3 &diffuseColor = Light::DefaultColor,
                   const glm::vec3 &specularColor = Light::DefaultColor,
                   const Attenuation &attenuation = Attenuation());

        virtual void setUniformsInShader(std::shared_ptr<BaseShader> shader, const std::string &prefix) override;

        Attenuation mAttenuation;
    };
}