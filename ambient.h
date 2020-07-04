#pragma once


#include "mesh.h"
#include <functional>


bool ambientOcclusion(std::vector<TexturedMesh>& meshes,
    std::function<bool(float)> progress,
    int sampleCnt = 20);
