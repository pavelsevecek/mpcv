#pragma once

#include "camera.h"
#include "mesh.h"
#include "pvl/UniformGrid.hpp"
#include <functional>

class FrameBufferWidget;

namespace Mpcv {

struct Pixel {
    Pvl::Vec3f color = Pvl::Vec3f(0);
    int weight = 0;

    void add(const Pvl::Vec3f& c) {
        color = (weight * color + c) / (weight + 1);
        ++weight;
    }
};


using FrameBuffer = Pvl::UniformGrid<Pixel, 2>;

void renderMeshes(FrameBufferWidget* widget,
    const std::vector<TexturedMesh*>& meshes,
    const Pvl::Vec3f& dirToSun,
    const Camera camera,
    const Srs& srs);

bool ambientOcclusion(std::vector<TexturedMesh>& meshes,
    std::function<bool(float)> progress,
    int sampleCnt = 20);

} // namespace Mpcv
