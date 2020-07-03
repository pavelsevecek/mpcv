#include "ambient.h"
#include "bvh.h"
#include <chrono>
#include <tbb/tbb.h>

void computeAmbientOcclusion(Mesh& mesh, std::function<bool(float)> progress, int sampleCnt) {
    Pvl::Bvh<Pvl::BvhTriangle> bvh(10);
    float scale;
    progress(0);
    {
        Pvl::Box3f box;
        std::vector<Pvl::BvhTriangle> triangles;
        for (const Mesh::Face& f : mesh.faces) {
            Pvl::Vec3f v1 = mesh.vertices[f[0]];
            Pvl::Vec3f v2 = mesh.vertices[f[1]];
            Pvl::Vec3f v3 = mesh.vertices[f[2]];
            triangles.emplace_back(v1, v2, v3);
            box.extend(mesh.vertices[f[0]]);
        }
        bvh.build(std::move(triangles));
        scale = std::max(box.size()[0], box.size()[1]);
    }
    // ad hoc
    progress(2);
    float eps = 1.e-3 * scale;
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::vector<Pvl::Vec3f> normals(mesh.vertices.size(), Pvl::Vec3f(0));
    tbb::parallel_for(std::size_t(0), mesh.faces.size(), [&](std::size_t fi) {
        const Mesh::Face& f = mesh.faces[fi];
        const Pvl::Vec3f n = mesh.normal(fi) * mesh.area(fi);
        normals[f[0]] += n;
        normals[f[1]] += n;
        normals[f[2]] += n;
    });
    if (progress(4)) {
        return;
    }


    mesh.colors.resize(mesh.vertices.size());
    // std::random_device rd;
    // tbb::enumerable_thread_specific<Rng> rng([&rd] { return rd(); });
    tbb::atomic<int> computedCnt = 0;
    int progressStep = mesh.vertices.size() / 100;
    tbb::mutex mutex;
    tbb::atomic<bool> cancelled = false;
    tbb::parallel_for(std::size_t(0), mesh.vertices.size(), std::size_t(1), [&](std::size_t vi) {
        if (cancelled) {
            return;
        }
        int nonOccludedCnt = 0;
        Pvl::Vec3f n = Pvl::normalize(normals[vi]);
        for (int x = 0; x < sampleCnt; ++x) {
            for (int y = 0; y < sampleCnt; ++y) {
                Pvl::Vec3f dir = sampleUnitSphere((x + 0.5f) / sampleCnt, (y + 0.5f) / sampleCnt);
                Pvl::Ray ray(mesh.vertices[vi] + eps * n, dir);
                if (!bvh.isOccluded(ray)) {
                    nonOccludedCnt++;
                }
            }
        }
        float rati = std::min(1.75f * float(nonOccludedCnt) / Pvl::sqr(sampleCnt), 1.f);
        uint8_t value = uint8_t(rati * 255);
        mesh.colors[vi] = Color(value);

        computedCnt++;
        if (computedCnt % progressStep == 0) {
            float value = 4 + 96 * float(computedCnt) / mesh.vertices.size();
            tbb::mutex::scoped_lock lock(mutex);
            if (progress(value)) {
                cancelled = true;
                mesh.colors = {};
                return;
            }
        }
    });
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "AO calculated in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms"
              << std::endl;
}
