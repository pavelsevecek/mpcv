#include "renderer.h"
#include "QCoreApplication"
#include "bvh.h"
#include "coordinates.h"
#include "framebuffer.h"
#include "pvl/Box.hpp"
#include "pvl/UniformGrid.hpp"
#include "pvl/Utils.hpp"
#include <QImage>
#include <QProgressDialog>
#include <random>
#ifdef HAS_OIDN
#include <OpenImageDenoise/oidn.hpp>
#endif

#include "sun-sky/SunSky.h"


namespace Mpcv {

struct MonoSunSky {
    Pvl::Vec3f skyIntensity = Pvl::Vec3f(1.f, 1.f, 1.2f);
    Pvl::Vec3f sunIntensity = Pvl::Vec3f(1.f, 1.f, 0.8f);

    void update(const Pvl::Vec3f&) {}

    Pvl::Vec3f evalSky(const Pvl::Vec3f&) const {
        return skyIntensity;
    }

    Pvl::Vec3f evalSun(const Pvl::Vec3f&) const {
        return skyIntensity;
    }
};

struct HosekWilkieSunSky {
    SSLib::cSunSkyHosek hosek;
    float units = 5.e-4f;

    void update(const Pvl::Vec3f& sunDir) {
        Vec3f dirToSun(sunDir[0], sunDir[1], sunDir[2]);
        hosek.Update(dirToSun, 2.5f);
    }

    Pvl::Vec3f evalSky(const Pvl::Vec3f& dir) const {
        Vec3f sky = hosek.SkyRGB(Vec3f(dir[0], dir[1], dir[2]));
        return units * Pvl::Vec3f(toGamut(sky[0]), toGamut(sky[1]), toGamut(sky[2]));
    }

    Pvl::Vec3f evalSun(const Pvl::Vec3f& dirToSun) const {
        Vec3f sun = SSLib::SunRGB(dirToSun[2]);
        return units * Pvl::Vec3f(toGamut(sun[0]), toGamut(sun[1]), toGamut(sun[2]));
    }

private:
    inline float toGamut(const float x) const {
        return std::max(x, 0.f);
    }
};


struct Scene {
    float albedo = 0.2f;

    float skyMult = 0.5f;
    float sunMult = 0.5f;
    Pvl::Vec3f sunDir = Pvl::normalize(Pvl::Vec3f(2, 5, 6));

    float sunRadius = 0.25f * M_PI / 180.f;

    HosekWilkieSunSky sunSky;

    struct Light {
        Pvl::Vec3f pos;
        Pvl::Vec3f intensity = 10 * Pvl::Vec3f(1.f, 1.f, 0.1f);
        float cosAngle = std::cos(0.3);

        Light() = default;
        Light(const Pvl::Vec3f& pos)
            : pos(pos) {}
        Light(float x, float y, float z)
            : pos(x, y, z) {}
    };

    std::vector<Light> lights;

    Scene(const Pvl::Vec3f& dirToSun) {
        sunDir = dirToSun;
        sunSky.update(sunDir);
#if 0
        lights.emplace_back(206.162094116, 178.98638916, 11.0727357864);
        lights.emplace_back(200.246337891, 164.296463013, 11.0424947739);
        lights.emplace_back(220.783050537, 156.128082275, 10.9192733765);
        lights.emplace_back(237.933807373, 153.780349731, 10.9892902374);
        lights.emplace_back(255.509719849, 158.282302856, 10.9857540131);
        lights.emplace_back(276.554138184, 169.323303223, 7.82737731934);
        lights.emplace_back(298.892150879, 185.251159668, 7.8238067627);
        lights.emplace_back(298.794189453, 185.307998657, 7.85432815552);
        lights.emplace_back(314.957183838, 196.964797974, 7.8002948761);
        lights.emplace_back(282.777008057, 146.740844727, 5.80917549133);
        lights.emplace_back(282.148498535, 133.335128784, 5.87916660309);
        lights.emplace_back(281.093078613, 131.42829895, 4.428399086);
        lights.emplace_back(300.863861084, 137.872619629, 5.97756099701);
        lights.emplace_back(356.055999756, 72.9139251709, 5.78705978394);

      lights.emplace_back(   222.284057617, 174.936965942, 10.1498947144);
         lights.emplace_back(224.596817017, 183.816253662, 0.127750396729);
        lights.emplace_back( 234.100036621, 173.430480957, 10.0278778076);
      lights.emplace_back(   247.690917969, 175.376083374, 9.91175270081);
#endif
    }

    //    std::vector<Pvl::Vec3f> vertexNormals;
    // Pvl::Vec3f lightPos = Pvl::Vec3f(-50, 25, 100);

    /*  Scene(const std::vector<TexturedMesh*>& meshes) {
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
      }*/
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

inline Pvl::Vec2f sampleUnitDisc(float x, float y) {
    float r = std::sqrt(x);
    float phi = 2.f * M_PI * y;
    return Pvl::Vec2f(r * cos(phi), r * cos(phi));
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

inline float distanceToSegment(const Pvl::Vec3f& v, const Pvl::Vec3f& a, const Pvl::Vec3f& b) {
    const Pvl::Vec3f& ab = b - a;
    const Pvl::Vec3f& av = v - a;
    return Pvl::norm(Pvl::crossProd(ab, av)) / Pvl::norm(ab);
}


inline float edgesShader(const Pvl::Vec3f& v, const std::array<Pvl::Vec3f, 3>& tri) {
    const float eps = 0.01;
    for (int i = 0; i < 3; ++i) {
        const Pvl::Vec3f& a = tri[i];
        const Pvl::Vec3f& b = tri[(i + 1) % 3];
        if (distanceToSegment(v, a, b) < eps) {
            return 0.f;
        }
    }
    return 1.f;
}

inline float vertexShader(const Pvl::Vec3f& v, const std::array<Pvl::Vec3f, 3>& tri) {
    const float eps = 0.01;
    for (int i = 0; i < 3; ++i) {
        if (Pvl::norm(v - tri[i]) < eps) {
            return 0.f;
        }
    }
    return 1.f;
}

Pvl::Vec3f radiance(const Scene& scene,
    const Mpcv::Ray& ray,
    const Mpcv::Bvh<Mpcv::BvhTriangle>& bvh,
    Rng& rng,
    const RenderWire wire,
    const int depth = 0) {
    float eps = 0.01f;
    Mpcv::IntersectionInfo is;
    if (bvh.getFirstIntersection(ray, is)) {
        const Mpcv::BvhTriangle* tri = static_cast<const Mpcv::BvhTriangle*>(is.object);
        const Pvl::Vec3f pos = ray.origin() + is.t * ray.direction();

        const Pvl::Vec3f normal = tri->normal();
        Pvl::Vec3f result(0.f);

        // Pvl::Vec3f uvw = barycentric(pos, tri->getTriangle());
        float albedo = scene.albedo;
        switch (wire) {
        case RenderWire::DOTS:
            albedo *= vertexShader(pos, tri->getTriangle());
            break;
        case RenderWire::EDGES:
            albedo *= edgesShader(pos, tri->getTriangle());
            break;
        case RenderWire::NOTHING:
            break;
        }
        // GI
        if (depth == 0) {
            Pvl::Mat33f rotator = Pvl::getRotatorTo(normal);
            int numGiSamples = 10;
            for (int i = 0; i < numGiSamples; ++i) {
                Pvl::Vec3f outDir = Pvl::prod(rotator, sampleUnitHemiSphere(rng(), rng()));
                Pvl::Vec3f gi =
                    radiance(scene, Mpcv::Ray(pos + eps * outDir, outDir), bvh, rng, wire, depth + 1);
                float bsdfCos = albedo * std::max(Pvl::dotProd(outDir, normal), 0.f);
                result += gi * bsdfCos / numGiSamples;
            }
        }

        // direct lighting
        Pvl::Mat33f rotator = Pvl::getRotatorTo(scene.sunDir);
        Pvl::Vec2f xy = scene.sunRadius * sampleUnitDisc(rng(), rng());
        Pvl::Vec3f dirToSun = Pvl::prod(rotator, Pvl::normalize(Pvl::Vec3f(xy[0], xy[1], 1.f)));
        if (!bvh.isOccluded(Mpcv::Ray(pos + eps * dirToSun, dirToSun))) {
            result += albedo * scene.sunMult * scene.sunSky.evalSun(dirToSun) *
                      std::max(Pvl::dotProd(normal, dirToSun), 0.f);
        }

        for (const Scene::Light& light : scene.lights) {
            const float distToLight = Pvl::norm(light.pos - pos);
            if (distToLight > 50) {
                continue;
            }
            const Pvl::Vec3f dirToLight = (light.pos - pos) / distToLight;
            /// \todo range-limited occlusion instead
            bool hit = bvh.getFirstIntersection(Mpcv::Ray(pos + eps * dirToLight, dirToLight), is);
            bool visible = !hit || is.t > distToLight - 1.f;
            bool illuminates = dirToLight[2] > 0; // light.cosAngle;
            if (visible && illuminates) {
                Pvl::Vec3f intensity = light.intensity * std::pow(dirToLight[2], 20.f);
                result += albedo * intensity * std::max(Pvl::dotProd(normal, dirToLight), 0.f);
            }
        }
        return result;
    } else {
        return scene.skyMult * scene.sunSky.evalSky(ray.direction());
    }
}

#ifdef HAS_OIDN
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
#else
void denoise(FrameBuffer&) {}
#endif

void renderMeshes(FrameBufferWidget* frame,
    const std::vector<TexturedMesh*>& meshes,
    const Pvl::Vec3f& dirToSun,
    const Camera camera,
    const RenderWire wire) {
    std::cout << "Starting the renderer" << std::endl;
    Scene scene(dirToSun);

    /// \todo deduplicate

    Mpcv::Bvh<Mpcv::BvhTriangle> bvh(10);

    float scale = 0.f;
    // progress(0);
    std::vector<Mpcv::BvhTriangle> triangles;
    std::size_t totalFaces = 0;
    int index = 0;
    for (const TexturedMesh* mesh : meshes) {
        totalFaces += mesh->faces.size();

        Pvl::Box3f box;
        SrsConv meshToRef(mesh->srs, camera.srs());
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
            return frame->cancelled();
        });

        Pvl::ParallelFor<Pvl::ParallelTag>()(0, dims[1], [&](int y) {
            if (frame->cancelled()) {
                return;
            }
            Rng& rng = threadRng.local();
            for (int x = 0; x < dims[0]; ++x) {
                if (meter.inc()) {
                    return;
                }
                Pvl::Vec2i pix(x, y);
                float dx = rng();
                float dy = rng();
                CameraRay cameraRay = camera.project(Pvl::Vec2f(x + dx, y + dy));
                Mpcv::Ray ray(cameraRay.origin, cameraRay.dir);
                Pvl::Vec3f color = radiance(scene, ray, bvh, rng, wire);
                framebuffer(pix).add(color);
            }
        });
        if (frame->cancelled()) {
            return;
        }
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


/*inline Pvl::Vec3f sampleUnitSphere(float x, float y) {
    const float phi = x * 2.f * M_PI;
    const float z = y * 2.f - 1.f;
    const float u = std::sqrt(1.f - z * z);
    return Pvl::Vec3f(u * std::cos(phi), u * std::sin(phi), z);
}*/

/*inline Pvl::Vec3f sampleUnitHemiSphere(float x, float y) {
    const float phi = x * 2.f * M_PI;
    const float z = y;
    const float u = std::sqrt(1.f - z * z);
    return Pvl::Vec3f(u * std::cos(phi), u * std::sin(phi), z);
}*/

bool ambientOcclusion(std::vector<TexturedMesh>& meshes,
    std::function<bool(float)> progress,
    int sampleCntX,
    int sampleCntY) {
    Mpcv::Bvh<Mpcv::BvhTriangle> bvh(10);
    Srs referenceSrs = meshes.front().srs;

    float scale = 0.f;
    progress(0);
    std::vector<Mpcv::BvhTriangle> triangles;
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
                for (int x = 0; x < sampleCntX; ++x) {
                    for (int y = 0; y < sampleCntY; ++y) {
                        Pvl::Vec3f dir =
                            sampleUnitHemiSphere((x + 0.5f) / sampleCntX, (y + 0.5f) / sampleCntY);
                        dir = Pvl::prod(rotator, dir);
                        Pvl::Vec3f origin = meshToRef(0.99 * mesh.vertices[vi] + 0.01 * centroid);
                        Mpcv::Ray ray(origin + eps * n, dir);
                        if (!bvh.isOccluded(ray)) {
                            nonOccludedCnt++;
                        }
                    }
                }

                float rati = float(nonOccludedCnt) / (sampleCntX * sampleCntY);
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

} // namespace Mpcv
