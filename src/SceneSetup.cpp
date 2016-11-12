#include "SceneSetup.hpp"

#include <json.hpp>

using json = nlohmann::json;

glm::vec3 arrayToVector(const json &j) {
    assert(j.is_array());

    glm::vec3 vec;
    vec.x = j.at(0);
    vec.y = j.at(1);
    vec.z = j.at(2);

    return vec;
}

pbd::SceneSetup pbd::SceneSetup::LoadFromJsonString(const std::string &str) {
    json j = json::parse(str.c_str());

    SceneSetup setup;

    setup.name = j["name"];

    {
        json jcamera = j["camera"];
        setup.camera.position = arrayToVector(jcamera["position"]);
        setup.camera.fovY = jcamera["fovY"];
    }

    {
        json jshaders = j["shaders"];
        for (const auto &jshader : jshaders) {
            ShaderConfig shader;

            shader.name = jshader["name"];
            shader.vertex = jshader["vertex"];
            shader.fragment = jshader["fragment"];

            setup.shaders.push_back(shader);
        }
        setup.shaders.shrink_to_fit();
    }

    {
        json jlights = j["lights"];
        for (const auto &jlight : jlights) {
            ColorConfig color;
            color.ambient = arrayToVector(jlight["color"]["ambient"]);
            color.diffuse = arrayToVector(jlight["color"]["diffuse"]);
            color.specular = arrayToVector(jlight["color"]["specular"]);

            if (jlight["type"] == "DIRECTIONAL") {
                DirectionalLightConfig light;
                light.color = color;
                light.direction = arrayToVector(jlight["direction"]);

                setup.directionalLights.push_back(light);
            } else if (jlight["type"] == "POINT") {
                PointLightConfig light;
                light.color = color;
                light.position = arrayToVector(jlight["position"]);

                AttenuationConfig att;
                att.linear = jlight["attenuation"]["linear"];
                att.quadratic = jlight["attenuation"]["quadratic"];
                light.attenuation = att;

                setup.pointLights.push_back(light);
            }
        }
        setup.pointLights.shrink_to_fit();
        setup.directionalLights.shrink_to_fit();
    }

    {
        json jmeshes = j["meshes"];
        for (const auto &jmesh : jmeshes) {
            MeshConfig mesh;

            mesh.isCloth = jmesh["isCloth"];
            mesh.path = jmesh["path"];
            mesh.shader = jmesh["shader"];
            mesh.position = arrayToVector(jmesh["position"]);
            mesh.orientation = arrayToVector(jmesh["orientation"]);
            mesh.scale = jmesh["scale"];

            setup.meshes.push_back(mesh);
        }
        setup.meshes.shrink_to_fit();
    }

    return setup;
}
