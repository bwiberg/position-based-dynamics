#include "ClothSimParams.hpp"

#include <fstream>
#include <bwgl/bwgl.hpp>
#include <json.hpp>

using json = nlohmann::json;

namespace pbd {
    ClothSimParams ClothSimParams::ReadFromFile(const std::string &filename) {
        std::string file = "";
        if (!bwgl::TryReadFromFile(filename, file)) return ClothSimParams();

        json j = json::parse(file.c_str());
        ClothSimParams params;

        params.deltaTime    = j["deltaTime"];
        params.numSubSteps  = j["numSubSteps"];
        params.k_bend       = j["k_bend"];
        params.k_stretch    = j["k_stretch"];

        return params;
    }

    void ClothSimParams::writeToFile(const std::string &filename) {
        json j;

        j["deltaTime"]      = deltaTime;
        j["numSubSteps"]    = numSubSteps;
        j["k_bend"]         = k_bend;
        j["k_stretch"]      = k_stretch;

        std::ofstream file(filename);

        file << j.dump(2) << std::endl;
        file.close();
    }
}

