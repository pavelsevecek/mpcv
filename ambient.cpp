#include "ambient.h"
#include "bvh.h"
#include "coordinates.h"
#include "pvl/Matrix.hpp"
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

void ambientOcclusion(std::vector<TexturedMesh>& meshes, std::function<bool(float)> progress, int sampleCnt) {
    Pvl::Bvh<Pvl::BvhTriangle> bvh(10);
    Srs referenceSrs = meshes.front().srs;

    float scale;
    Pvl::Box3f box;
    progress(0);
    std::vector<Pvl::BvhTriangle> triangles;
    for (const TexturedMesh& mesh : meshes) {
        SrsConv meshToRef(mesh.srs, referenceSrs);

        for (const TexturedMesh::Face& f : mesh.faces) {
            Pvl::Vec3f v1 = meshToRef(mesh.vertices[f[0]]);
            Pvl::Vec3f v2 = meshToRef(mesh.vertices[f[1]]);
            Pvl::Vec3f v3 = meshToRef(mesh.vertices[f[2]]);
            triangles.emplace_back(v1, v2, v3);
            box.extend(mesh.vertices[f[0]]);
        }
    }
    bvh.build(std::move(triangles));
    triangles = {};

    scale = std::max(box.size()[0], box.size()[1]);

    // ad hoc
    progress(2);
    const float eps = 1.e-4 * scale;
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    tbb::tbb_thread::id mainThreadId = tbb::this_tbb_thread::get_id();
    for (TexturedMesh& mesh : meshes) {
        SrsConv meshToRef(mesh.srs, referenceSrs);

        mesh.ao.resize(3 * mesh.faces.size());

        int progressStep = std::max<std::size_t>(mesh.faces.size() / 100, 10);
        tbb::atomic<int> computedCnt = 0;
        tbb::atomic<int> nextProgress = progressStep;
        tbb::mutex mutex;
        tbb::atomic<bool> cancelled = false;
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
                        Pvl::Ray ray(meshToRef(0.99 * mesh.vertices[vi] + 0.01 * centroid) + eps * n, dir);
                        if (!bvh.isOccluded(ray)) {
                            nonOccludedCnt++;
                        }
                    }
                }

                float rati = float(nonOccludedCnt) / Pvl::sqr(sampleCnt);
                uint8_t value = uint8_t(rati * 255);
                mesh.ao[3 * fi + i] = value;
            }

            computedCnt++;
            tbb::tbb_thread::id threadId = tbb::this_tbb_thread::get_id();
            if (threadId == mainThreadId && computedCnt > nextProgress) {
                float value = 4 + 96 * float(computedCnt) / mesh.faces.size();
                nextProgress += progressStep;
                tbb::mutex::scoped_lock lock(mutex);
                if (progress(value)) {
                    cancelled = true;
                    mesh.colors = {};
                    return;
                }
            }
        });
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "AO calculated in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms"
              << std::endl;
}
