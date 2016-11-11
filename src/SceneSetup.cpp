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
        json camera = j["camera"];
        setup.camera.position = arrayToVector(camera["position"]);
        setup.camera.fovY = camera["fovY"];
    }

    {
        json shaders = j["shaders"];
        for (const auto &jshader : shaders) {
            ShaderConfig shader;

            shader.name = jshader["name"];
            shader.vertex = jshader["vertex"];
            shader.fragment = jshader["fragment"];

            setup.shaders.push_back(shader);
        }
        setup.shaders.shrink_to_fit();
    }

    {
        json meshes = j["meshes"];
        for (const auto &jmesh : meshes) {
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
