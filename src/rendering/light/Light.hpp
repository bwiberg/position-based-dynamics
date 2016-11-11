#pragma once

#include "rendering/BaseShader.hpp"

namespace clgl {
    namespace LightType {
        const std::string AMBIENT       = "ambient";
        const std::string DIRECTIONAL   = "directional";
        const std::string POINT         = "point";
    }

    class Light {
    public:
        Light(const glm::vec3 &color, float intensity, const std::string &type);

        virtual void setUniformsInShader(std::shared_ptr<BaseShader> shader,
                                         const std::string &prefix = "") = 0;

        const glm::vec3 &getColor() const;

        float getIntensity() const;

        const std::string &getType() const;

        void setColor(const glm::vec3 &color);

        void setIntensity(float intensity);

    protected:
        glm::vec3 mColor;

        float mIntensity;

    private:
        const std::string mType;
    };

    inline Light::Light(const glm::vec3 &color, float intensity, const std::string &type)
            : mColor(color), mIntensity(intensity), mType(type) {}

    inline const glm::vec3 &Light::getColor() const {
        return mColor;
    }

    inline float Light::getIntensity() const {
        return mIntensity;
    }

    inline const std::string &Light::getType() const {
        return mType;
    }

    inline void Light::setColor(const glm::vec3 &color) {
        mColor = color;
    }

    inline void Light::setIntensity(float intensity) {
        mIntensity = intensity;
    }
}