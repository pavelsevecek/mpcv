#include "ambient.h"
#include "coordinates.h"
#include "bvh.h"
#include <chrono>
#include <tbb/tbb.h>
#include <map>

struct Edge {
    int i1, i2;

    Edge() = default;

    Edge(int a, int b) 
        : i1(a), i2(b) {
        if (i2 < i1) {
            std::swap(i1, i2);
        }
    }

    bool operator<(const Edge& other) const {
        return std::make_pair(i1, i2) < std::make_pair(other.i1, other.i2);
    }
};

void computeAmbientOcclusion(std::vector<Mesh>& meshes, std::function<bool(float)> progress, int sampleCnt) {
    Pvl::Bvh<Pvl::BvhTriangle> bvh(10);
    Srs referenceSrs = meshes.front().srs;

    float scale;
    Pvl::Box3f box;
    progress(0);
    std::vector<Pvl::BvhTriangle> triangles;
    for (const Mesh& mesh : meshes) {
        SrsConv meshToRef(mesh.srs, referenceSrs);

        for (const Mesh::Face& f : mesh.faces) {
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
    float eps = 1.e-3 * scale;
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
/*    if (progress(4)) {
        return;
    }*/
    /*std::vector<bool> isBoundary(mesh.vertices.size(), false);
    for (auto&p : edgeMap) {
        if (p.second == 1) {
            isBoundary[p.first.i1] = isBoundary[p.first.i2] = true;
        }
    }
    edgeMap.clear();*/

    tbb::tbb_thread::id mainThreadId = tbb::this_tbb_thread::get_id();
    for (Mesh& mesh : meshes ){
        SrsConv meshToRef(mesh.srs, referenceSrs);

        std::vector<Pvl::Vec3f> normals(mesh.vertices.size(), Pvl::Vec3f(0));
        //std::map<Edge, int> edgeMap;

        for (std::size_t fi = 0; fi < mesh.faces.size(); ++fi) {
            const Mesh::Face& f = mesh.faces[fi];
            const Pvl::Vec3f n = mesh.normal(fi) * mesh.area(fi);
            normals[f[0]] += n;
            normals[f[1]] += n;
            normals[f[2]] += n;
          /*  edgeMap[Edge(f[0], f[1])]++;
            edgeMap[Edge(f[1], f[2])]++;
            edgeMap[Edge(f[2], f[0])]++;*/
        }

        mesh.colors.resize(mesh.vertices.size());
        // std::random_device rd;
        // tbb::enumerable_thread_specific<Rng> rng([&rd] { return rd(); });
        int progressStep = std::max<std::size_t>(mesh.vertices.size() / 100, 10);
        tbb::atomic<int> computedCnt = 0;
        tbb::atomic<int> nextProgress = progressStep;
        tbb::mutex mutex;
        tbb::atomic<bool> cancelled = false;        
        tbb::parallel_for(std::size_t(0), mesh.vertices.size(), [&](std::size_t vi) {
            if (cancelled) {
                return;
            }
            int nonOccludedCnt = 0;
            Pvl::Vec3f n;
            if (Pvl::norm(normals[vi]) < 1.e-20) {
                n = Pvl::Vec3f(0);
            } else {
                n = Pvl::normalize(normals[vi]);
            }
            for (int x = 0; x < sampleCnt; ++x) {
                for (int y = 0; y < sampleCnt; ++y) {
                    Pvl::Vec3f dir = sampleUnitSphere((x + 0.5f) / sampleCnt, (y + 0.5f) / sampleCnt);
                    Pvl::Ray ray(meshToRef(mesh.vertices[vi]) + eps * n, dir);
                    if (!bvh.isOccluded(ray)) {
                        nonOccludedCnt++;
                    }
                }
            }
            /*if (isBoundary[vi]) {
                nonOccludedCnt /= 2;
            }*/
            float rati = std::min(1.75f * float(nonOccludedCnt) / Pvl::sqr(sampleCnt), 1.f);
            uint8_t value = uint8_t(rati * 255);
            mesh.colors[vi] = Color(value);

            computedCnt++;
            tbb::tbb_thread::id threadId = tbb::this_tbb_thread::get_id();
            if (threadId == mainThreadId && computedCnt > nextProgress) {
                float value = 4 + 96 * float(computedCnt) / mesh.vertices.size();
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
