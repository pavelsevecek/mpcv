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

    Color get(float exposure) const {
        if (weight > 0) {
            Color result;
            for (int c = 0; c < 3; ++c) {
                float value = exposure * color[c];
                float clamped = std::max(std::min(value, 1.f), 0.f);
                result[c] = uint8_t(std::pow(clamped, 1.f / 2.2f) * 255.f);
            }
            return result;
        } else {
            return Color(0);
        }
    }
};


using FrameBuffer = Pvl::UniformGrid<Pixel, 2>;

void renderMeshes(FrameBufferWidget* widget,
    const std::vector<TexturedMesh*>& meshes,
    const Camera& camera,
    const Srs& srs);
