#include "DirectionalLight.hpp"

namespace clgl {
    DirectionalLight::DirectionalLight(const glm::vec3 &ambientColor,
                                       const glm::vec3 &diffuseColor,
                                       const glm::vec3 &specularColor,
                                       const glm::vec3 &lightDirection)
            : Light(LightType::DIRECTIONAL, ambientColor, diffuseColor, specularColor),
              mLightDirection(lightDirection) {}

    void DirectionalLight::setUniformsInShader(std::shared_ptr<clgl::BaseShader> shader,
                                               const std::string &prefix) {
        Light::setUniformsInShader(shader, prefix);

        glm::vec3 transformedLightDirection = glm::vec3(getTransform() * glm::vec4(mLightDirection, 0.0f));
        shader->uniform(prefix + ".dir", transformedLightDirection);
    }
}
