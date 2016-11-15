#pragma once

#include <string>
#include <CL/cl.hpp>

namespace pbd {
    struct ClothSimParams {
        static ClothSimParams ReadFromFile(const std::string &filename);

        void writeToFile(const std::string &filename);

        // Number of PBD constraint projection steps
        cl_uint numSubSteps;

        // Time step (dt):
        cl_float deltaTime;

        // PBD stiffness constant for cloth stretch constraint
        cl_float k_stretch;

        // PBD stiffness constant for cloth bending constraint
        cl_float k_bend;
    };
}
