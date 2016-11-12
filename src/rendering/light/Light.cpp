#include "Light.hpp"

namespace clgl {
    const glm::vec3 Light::DefaultColor(1.0f, 1.0f, 1.0f);

    Light::Light(const std::string &type,
                 const glm::vec3 &ambientColor,
                 const glm::vec3 &diffuseColor,
                 const glm::vec3 &specularColor)
            : mType(type),
              mAmbientColor(ambientColor),
              mDiffuseColor(diffuseColor),
              mSpecularColor(specularColor) {}

    void Light::setUniformsInShader(std::shared_ptr<BaseShader> shader) {
        setUniformsInShader(shader, mType);
    }

    void Light::setUniformsInShader(std::shared_ptr<BaseShader> shader,
                                    const std::string &prefix) {
        shader->uniform(prefix + ".ambient", mAmbientColor);
        shader->uniform(prefix + ".diffuse", mDiffuseColor);
        shader->uniform(prefix + ".specular", mSpecularColor);
    }
}