#include "PointLight.hpp"

namespace clgl {
    PointLight::PointLight(const glm::vec3 &ambientColor,
                           const glm::vec3 &diffuseColor,
                           const glm::vec3 &specularColor,
                           const Attenuation &attenuation)
            : Light(LightType::POINT, mAmbientColor, mDiffuseColor, mSpecularColor),
              mAttenuation(attenuation) {}

    void PointLight::setUniformsInShader(std::shared_ptr<BaseShader> shader, const std::string &prefix) {
        Light::setUniformsInShader(shader, prefix);

        shader->uniform(prefix + ".att.linear", mAttenuation.linear);
        shader->uniform(prefix + ".att.quadratic", mAttenuation.quadratic);

        const glm::vec4 transformedLightPosition = getTransform() * glm::vec4(getPosition(), 1.0f);
        shader->uniform(prefix + ".position", transformedLightPosition);
    }
}
