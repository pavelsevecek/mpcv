#pragma once


#include "mesh.h"
#include <functional>


/// \brief Generates a random position on a unit sphere
inline Pvl::Vec3f sampleUnitSphere(float x, float y) {
    const float phi = x * 2.f * M_PI;
    const float z = y * 2.f - 1.f;
    const float u = std::sqrt(1.f - z * z);
    return Pvl::Vec3f(u * std::cos(phi), u * std::sin(phi), z);
}

void ambientOcclusion(std::vector<TexturedMesh>& meshes,
    std::function<bool(float)> progress,
    int sampleCnt = 20);
