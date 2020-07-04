#include "ambient.h"
#include "bvh.h"
#include "coordinates.h"
#include "pvl/Matrix.hpp"
#include "pvl/Utils.hpp"
#include <chrono>
#include <iostream>
#include <map>
#include <tbb/tbb.h>


inline Pvl::Vec3f sampleUnitSphere(float x, float y) {
    const float phi = x * 2.f * M_PI;
    const float z = y * 2.f - 1.f;
    const float u = std::sqrt(1.f - z * z);
    return Pvl::Vec3f(u * std::cos(phi), u * std::sin(phi), z);
}

inline Pvl::Vec3f sampleUnitHemiSphere(float x, float y) {
    const float phi = x * 2.f * M_PI;
    const float z = y;
    const float u = std::sqrt(1.f - z * z);
    return Pvl::Vec3f(u * std::cos(phi), u * std::sin(phi), z);
}

bool ambientOcclusion(std::vector<TexturedMesh>& meshes, std::function<bool(float)> progress, int sampleCnt) {
    Pvl::Bvh<Pvl::BvhTriangle> bvh(10);
    Srs referenceSrs = meshes.front().srs;

    float scale = 0.f;
    progress(0);
    std::vector<Pvl::BvhTriangle> triangles;
    std::size_t totalFaces = 0;
    for (const TexturedMesh& mesh : meshes) {
        totalFaces += mesh.faces.size();

        Pvl::Box3f box;
        SrsConv meshToRef(mesh.srs, referenceSrs);
        for (const TexturedMesh::Face& f : mesh.faces) {
            Pvl::Vec3f v1 = meshToRef(mesh.vertices[f[0]]);
            Pvl::Vec3f v2 = meshToRef(mesh.vertices[f[1]]);
            Pvl::Vec3f v3 = meshToRef(mesh.vertices[f[2]]);
            triangles.emplace_back(v1, v2, v3);

            if (scale == 0.f) {
                // compute box from the first mesh only
                box.extend(mesh.vertices[f[0]]);
            }
        }
        if (scale == 0.f) {
            scale = std::max(box.size()[0], box.size()[1]);
        }
    }
    bvh.build(std::move(triangles));
    triangles = {};

    // ad hoc
    progress(1);
    const float eps = 1.e-3f * scale;
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    auto meter = Pvl::makeProgressMeter(totalFaces, std::move(progress));
    tbb::atomic<bool> cancelled = false;
    for (TexturedMesh& mesh : meshes) {
        SrsConv meshToRef(mesh.srs, referenceSrs);

        mesh.ao.resize(3 * mesh.faces.size());

        tbb::parallel_for(std::size_t(0), mesh.faces.size(), [&](std::size_t fi) {
            if (cancelled) {
                return;
            }
            Pvl::Vec3f n = mesh.normal(fi);
            Pvl::Vec3f centroid = mesh.centroid(fi);
            Pvl::Mat33f rotator = Pvl::getRotatorTo(n);
            for (int i = 0; i < 3; ++i) {
                int nonOccludedCnt = 0;

                int vi = mesh.faces[fi][i];
                for (int x = 0; x < sampleCnt; ++x) {
                    for (int y = 0; y < sampleCnt; ++y) {
                        Pvl::Vec3f dir = sampleUnitHemiSphere((x + 0.5f) / sampleCnt, (y + 0.5f) / sampleCnt);
                        dir = Pvl::prod(rotator, dir);
                        Pvl::Vec3f origin = meshToRef(0.99 * mesh.vertices[vi] + 0.01 * centroid);
                        Pvl::Ray ray(origin + eps * n, dir);
                        if (!bvh.isOccluded(ray)) {
                            nonOccludedCnt++;
                        }
                    }
                }

                float rati = float(nonOccludedCnt) / Pvl::sqr(sampleCnt);
                uint8_t value = uint8_t(rati * 255);
                mesh.ao[3 * fi + i] = value;
            }

            if (meter.inc()) {
                cancelled = true;
                mesh.ao = {};
                return;
            }
        });

        if (cancelled) {
            return false;
        }
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "AO calculated in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms"
              << std::endl;
    return true;
}
