#include "renderer.h"
#include "QCoreApplication"
#include "bvh.h"
#include "coordinates.h"
#include "framebuffer.h"
#include "pvl/Box.hpp"
#include "pvl/UniformGrid.hpp"
#include "pvl/Utils.hpp"
#include <OpenImageDenoise/oidn.hpp>
#include <QImage>
#include <QProgressDialog>
#include <random>

struct Scene {
    float albedo = 0.75;

    Pvl::Vec3f skyIntensity = Pvl::Vec3f(0.25f, 0.25f, 0.3f);

    Pvl::Vec3f sunIntensity = Pvl::Vec3f(0.2, 0.2f, 0.15f);
    Pvl::Vec3f sunDir = Pvl::normalize(Pvl::Vec3f(1, 1, 5));

    float sunRadius = 0.05; // rads

    std::vector<Pvl::Vec3f> vertexNormals;
    // Pvl::Vec3f lightPos = Pvl::Vec3f(-50, 25, 100);

    Scene(const std::vector<TexturedMesh*>& meshes) {
        std::size_t totalVertices = 0;
        for (const TexturedMesh* mesh : meshes) {
            totalVertices += mesh->vertices.size();
        }
        vertexNormals.resize(totalVertices, Pvl::Vec3f(0));
        std::size_t offset = 0;
        for (const TexturedMesh* mesh : meshes) {
            for (std::size_t fi = 0; fi < mesh->faces.size(); ++fi) {
                Pvl::Vec3f n = mesh->normal(fi) * mesh->area(fi);
                vertexNormals[offset + mesh->faces[fi][0]] += n;
                vertexNormals[offset + mesh->faces[fi][1]] += n;
                vertexNormals[offset + mesh->faces[fi][2]] += n;
            }
            offset += mesh->vertices.size();
        }
        for (Pvl::Vec3f& n : vertexNormals) {
            float length = Pvl::norm(n);
            if (length > 1.e-20) {
                n /= length;
            } else {
                n = Pvl::Vec3f(0, 0, 1);
            }
        }
    }
};

struct Rng {
    std::mt19937 gen;
    std::uniform_real_distribution<float> dist;
    Rng(std::size_t seed)
        : gen(seed)
        , dist(0, 1) {}

    float operator()() {
        return dist(gen);
    }
};

/// \todo deduplicate
inline Pvl::Vec3f sampleUnitHemiSphere(float x, float y) {
    const float phi = x * 2.f * M_PI;
    const float z = y;
    const float u = std::sqrt(1.f - z * z);
    return Pvl::Vec3f(u * std::cos(phi), u * std::sin(phi), z);
}

inline Pvl::Vec3f barycentric(const Pvl::Vec3f& p, const std::array<Pvl::Vec3f, 3>& tri) {
    Pvl::Vec3f v0 = tri[1] - tri[0];
    Pvl::Vec3f v1 = tri[2] - tri[0];
    Pvl::Vec3f v2 = p - tri[0];
    float d00 = Pvl::dotProd(v0, v0);
    float d01 = Pvl::dotProd(v0, v1);
    float d11 = Pvl::dotProd(v1, v1);
    float d20 = Pvl::dotProd(v2, v0);
    float d21 = Pvl::dotProd(v2, v1);
    float denom = d00 * d11 - d01 * d01;
    if (denom != 0.f) {
        return Pvl::Vec3f(0.3333, 0.3333, 0.3333);
    } else {
        float v = (d11 * d20 - d01 * d21) / denom;
        float w = (d00 * d21 - d01 * d20) / denom;
        float u = 1.f - v - w;
        return Pvl::Vec3f(u, v, w);
    }
}

Pvl::Vec3f radiance(const Scene& scene,
    const Pvl::Ray& ray,
    const Pvl::Bvh<Pvl::BvhTriangle>& bvh,
    Rng& rng,
    const int depth = 0) {
    float eps = 0.01f;
    Pvl::IntersectionInfo is;
    if (bvh.getFirstIntersection(ray, is)) {
        const Pvl::BvhTriangle* tri = static_cast<const Pvl::BvhTriangle*>(is.object);
        const Pvl::Vec3f pos = ray.origin() + is.t * ray.direction();

        const Pvl::Vec3f normal = tri->normal();
        Pvl::Vec3f result(0.f);

        // GI
        if (depth == 0) {
            Pvl::Mat33f rotator = Pvl::getRotatorTo(normal);
            for (int i = 0; i < 10; ++i) {
                Pvl::Vec3f outDir = Pvl::prod(rotator, sampleUnitHemiSphere(rng(), rng()));
                Pvl::Vec3f gi = radiance(scene, Pvl::Ray(pos + eps * outDir, outDir), bvh, rng, depth + 1);
                float bsdfCos = scene.albedo * std::max(Pvl::dotProd(outDir, normal), 0.f);
                result += gi * bsdfCos;
            }
        }

        // direct lighting
        Pvl::Vec3f dirToLight = scene.sunDir;
        if (!bvh.isOccluded(Pvl::Ray(pos + eps * dirToLight, dirToLight))) {
            result += scene.sunIntensity * Pvl::dotProd(normal, dirToLight);
        }
        return result;
    } else {
        return scene.skyIntensity;
    }
}

void denoise(FrameBuffer& framebuffer) {
    oidn::DeviceRef device = oidn::newDevice();
    device.commit();

    oidn::FilterRef filter = device.newFilter("RT");
    int width = framebuffer.dimension()[0];
    int height = framebuffer.dimension()[1];
    filter.setImage("color", framebuffer.data(), oidn::Format::Float3, width, height, 0, sizeof(Pixel));
    filter.setImage("output", framebuffer.data(), oidn::Format::Float3, width, height, 0, sizeof(Pixel));
    filter.set("hdr", true);
    filter.commit();
    filter.execute();

    const char* errorMessage;
    if (device.getError(errorMessage) != oidn::Error::None) {
        std::cout << "Error: " << errorMessage << std::endl;
    }
}

void renderMeshes(FrameBufferWidget* frame,
    const std::vector<TexturedMesh*>& meshes,
    const Camera& camera,
    const Srs& srs) {
    std::cout << "Starting the renderer" << std::endl;
    Scene scene(meshes);

    /// \todo deduplicate

    Pvl::Bvh<Pvl::BvhTriangle> bvh(10);

    float scale = 0.f;
    // progress(0);
    std::vector<Pvl::BvhTriangle> triangles;
    std::size_t totalFaces = 0;
    int index = 0;
    for (const TexturedMesh* mesh : meshes) {
        totalFaces += mesh->faces.size();

        Pvl::Box3f box;
        SrsConv meshToRef(mesh->srs, srs);
        for (const TexturedMesh::Face& f : mesh->faces) {
            Pvl::Vec3f v1 = meshToRef(mesh->vertices[f[0]]);
            Pvl::Vec3f v2 = meshToRef(mesh->vertices[f[1]]);
            Pvl::Vec3f v3 = meshToRef(mesh->vertices[f[2]]);
            triangles.emplace_back(v1, v2, v3, index++);

            if (scale == 0.f) {
                // compute box from the first mesh only
                box.extend(mesh->vertices[f[0]]);
            }
        }
        if (scale == 0.f) {
            scale = std::max(box.size()[0], box.size()[1]);
        }
    }
    bvh.build(std::move(triangles));
    triangles = {};

    Pvl::Vec2i dims = camera.dimensions();
    std::random_device rd;
    tbb::enumerable_thread_specific<Rng> threadRng([&rd] { return rd(); });

    FrameBuffer framebuffer(dims);
    int numPasses = 10;
    for (int pass = 0; pass < numPasses; ++pass) {
        /*QProgressDialog dialog("Rendering - iteration " + QString::number(pass + 1), "Cancel", 0, 100,
        frame); dialog.setWindowModality(Qt::WindowModal); dialog.show();*/
        auto meter = Pvl::makeProgressMeter(dims[0] * dims[1], [&frame, pass](float prog) {
            // dialog.setValue(prog);
            // QCoreApplication::processEvents();
            // return dialog.wasCanceled();
            frame->setProgress(pass, prog);
            return false;
        });

        Pvl::ParallelFor<Pvl::ParallelTag>()(0, dims[1], [&](int y) {
            Rng& rng = threadRng.local();
            for (int x = 0; x < dims[0]; ++x) {
                meter.inc();
                Pvl::Vec2i pix(x, y);
                float dx = rng();
                float dy = rng();
                Ray cameraRay = camera.project(Pvl::Vec2f(x + dx, y + dy));
                Pvl::Ray ray(cameraRay.origin, cameraRay.dir);
                Pvl::Vec3f color = radiance(scene, ray, bvh, rng);
                framebuffer(pix).add(color);
            }
        });
        if (pass == numPasses - 1) {
            denoise(framebuffer);
        }
        Image image(dims);
        Pvl::ParallelFor<Pvl::ParallelTag>()(0, dims[1], [&](int y) {
            for (int x = 0; x < dims[0]; ++x) {
                Pvl::Vec2i pix(x, y);
                image(pix) = framebuffer(pix).color;
            }
        });
        frame->setImage(std::move(image));
    }
    // set complete
    frame->setProgress(10, 100);
}
