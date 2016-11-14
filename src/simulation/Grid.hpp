#pragma once

#include <CL/cl.hpp>
#include "util/cl_util.hpp"

namespace pbd {
    struct Grid {
        std::string getDefinesCL();

        cl_float3 halfDimensions;
        cl_float binSize;
        cl_uint3 binCount3D;
        cl_uint binCount;
    };

    inline std::string Grid::getDefinesCL() {
        using std::to_string;

        const std::string args[16] = {
                "halfDimsX",  to_string(halfDimensions.s[0]) + "f",
                "halfDimsY",  to_string(halfDimensions.s[1]) + "f",
                "halfDimsZ",  to_string(halfDimensions.s[2]) + "f",
                "binSize",          to_string(binSize) + "f",
                "binCountX",        to_string(binCount3D.s[0]),
                "binCountY",        to_string(binCount3D.s[1]),
                "binCountZ",        to_string(binCount3D.s[2]),
                "binCount",         to_string(binCount)
        };

        return util::ConvertToCLDefines(8, args);
    }
}
