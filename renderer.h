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

enum class RenderWire {
    NOTHING,
    DOTS,
    EDGES,
};

struct RenderSettings {
    Pvl::Vec2i resolution = Pvl::Vec2i(1024, 768);
    int numIters = 10;
    Pvl::Vec3f dirToSun = Pvl::normalize(Pvl::Vec3f(1.f, 1.f, 4.f));
    RenderWire wire = RenderWire::NOTHING;
    bool denoise = false;
};

void renderMeshes(FrameBufferWidget* widget,
                  const std::vector<TexturedMesh*>& meshes,
                  const Camera camera,
                  const RenderSettings& settings);

bool ambientOcclusion(std::vector<TexturedMesh>& meshes,
                      std::function<bool(float)> progress,
                      int sampleCntX = 20,
                      int sampleCntY = 10);

} // namespace Mpcv
