#pragma once

#include "camera.h"
#include "mesh.h"
#include "pvl/UniformGrid.hpp"

class FrameBufferWidget;

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
