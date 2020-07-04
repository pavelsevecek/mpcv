#pragma once

#include "camera.h"
#include "coordinates.h"
#include "mesh.h"
#include "pvl/Box.hpp"
#include "pvl/Optional.hpp"
#include "quaternion.h"
#include <GL/glu.h>
#include <QFileInfo>
#include <QImageWriter>
#include <QMouseEvent>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QWheelEvent>
#include <fstream>
#include <iostream>

using Triangle = std::array<Pvl::Vec3f, 3>;

inline bool intersection(const Ray& ray, const Triangle& tri, float& t) {
    // https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm#C++_Implementation

    const Pvl::Vec3f dir1 = tri[1] - tri[0];
    const Pvl::Vec3f dir2 = tri[2] - tri[0];
    const float eps = 1.e-12f;
    const Pvl::Vec3f h = Pvl::crossProd(ray.dir, dir2);
    const float a = Pvl::dotProd(dir1, h);
    if (a > -eps && a < eps) {
        return false;
    }
    const float f = 1.f / a;
    const Pvl::Vec3f s = ray.origin - tri[0];
    const float u = f * Pvl::dotProd(s, h);
    if (u < 0.f || u > 1.f) {
        return false;
    }
    const Pvl::Vec3f q = Pvl::crossProd(s, dir1);
    const float v = f * Pvl::dotProd(ray.dir, q);
    if (v < 0.f || u + v > 1.f) {
        return false;
    }
    t = f * Pvl::dotProd(dir2, q);
    return true;
}

inline bool intersection(const Ray& ray, const Pvl::Vec3f& point, float radius, float& t) {
    Pvl::Vector<double, 3> delta = Pvl::vectorCast<double>(point) - Pvl::vectorCast<double>(ray.origin);
    double cosPhi = Pvl::dotProd(delta, Pvl::vectorCast<double>(ray.dir));
    if (cosPhi < 0) {
        return false;
    }
    Pvl::Vector<double, 3> r0 = Pvl::vectorCast<double>(ray.dir) * cosPhi;
    if (Pvl::norm(r0 - delta) < radius) {
        t = cosPhi;
        return true;
    } else {
        return false;
    }
}

class OpenGLWidget : public QOpenGLWidget, public QOpenGLFunctions {
    Q_OBJECT

    struct MeshData {
        TexturedMesh mesh;
        Pvl::Box3f box;
        bool enabled = true;

        struct {
            std::vector<float> vertices;
            std::vector<float> normals;
            std::vector<float> uv;
            std::vector<uint8_t> colors;
        } vis;

        GLuint texture;
        GLuint vbo;

        bool pointCloud() const {
            return mesh.faces.empty();
        }
        bool hasNormals() const {
            // mesh always has (face) normals
            return !pointCloud() || !mesh.normals.empty();
        }
        bool hasColors() const {
            return !mesh.colors.empty();
        }
        bool hasTexture() const {
            // point cloud cannot have texture
            return !pointCloud() && !mesh.uv.empty();
        }
    };
    // Pvl::Optional<Triangle> selected;

    Camera camera_;
    Srs srs_;
    float fov_ = M_PI / 4.f;
    float pointSize_ = 2.f;
    float grid_ = 1.f;
    bool showGrid_ = false;

    bool enableMeshColors_ = false; // currently only used for AO, needs to be computed first
    bool enableTextures_ = true;

    std::map<const void*, MeshData> meshes_;
    bool wireframe_ = false;
    bool dots_ = false;
    bool vbos_ = true;

    struct {
        QPoint pos0;
        ArcBall ab;
        Camera state;
    } mouse_;

public:
    OpenGLWidget(QWidget* parent)
        : QOpenGLWidget(parent) {}

    virtual void initializeGL() override;

    virtual void resizeGL(const int width, const int height) override;

    virtual void paintGL() override;

    void view(const void* handle, TexturedMesh&& mesh);

    void toggle(const void* handle, bool on) {
        meshes_[handle].enabled = on;
        update();
    }

    void wireframe(const bool on) {
        wireframe_ = on;
        update();
    }

    void dots(const bool on) {
        dots_ = on;
        update();
    }

    void deleteMesh(const void* handle) {
        if (handle == nullptr) {
            // nothing?
            return;
        }
        MeshData& mesh = meshes_.at(handle);
        if (vbos_) {
            glDeleteBuffers(1, &mesh.vbo);
        }
        if (meshes_.at(handle).hasTexture()) {
            glDeleteTextures(1, &mesh.texture);
        }
        meshes_.erase(handle);
        update();
    }

    void laplacianSmooth();

    void simplify();

    void repair();

    void computeAmbientOcclusion(std::function<bool(float)> progress);

    void enableMeshColors(bool on) {
        enableMeshColors_ = on;
        if (on) {
            enableTextures_ = false;
        }
        update();
    }

    void enableTextures(bool on) {
        enableTextures_ = on;
        if (on) {
            enableMeshColors_ = false;
        }
        update();
    }

    void grid(bool on) {
        showGrid_ = on;
        update();
    }

    void resetCamera() {
        // find first enabled
        for (const auto& p : meshes_) {
            const MeshData& mesh = p.second;
            if (!mesh.enabled) {
                continue;
            }

            SrsConv conv(mesh.mesh.srs, srs_);
            Pvl::Vec3f center = conv(mesh.box.center());
            float scale = std::max(mesh.box.size()[0], mesh.box.size()[1]);
            float zoom = 1.5 * scale;

            camera_ = Camera(center + Pvl::Vec3f(0, 0, zoom),
                center,
                Pvl::Vec3f(0, 1, 0),
                fov_,
                Pvl::Vec2i(width(), height()));

            double gridBase = 0.2 * scale;
            grid_ = std::pow(10., std::floor(std::log10(gridBase)));
            if (5.f * grid_ < gridBase) {
                grid_ *= 5.f; // allow 5e10^n grids
            } else if (2.f * grid_ < gridBase) {
                grid_ *= 2.f; // allow 2e10^n grids
            }
            std::cout << "Grid = " << grid_ << std::endl;

            update();

            return;
        }
    }

    void screenshot(const QString& file) {
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadBuffer(GL_FRONT);
        std::vector<uint8_t> pixels(width() * height() * 3);
        glReadPixels(0, 0, width(), height(), GL_BGR_EXT, GL_UNSIGNED_BYTE, pixels.data());
        QImage image(pixels.data(), width(), height(), width() * 3, QImage::Format_RGB888);
        QImageWriter writer(file);
        writer.write(std::move(image).mirrored().rgbSwapped());
    }

    void saveMesh(const QString& file, const void* handle) {
        std::ofstream ofs(file.toStdString());
        savePly(ofs, meshes_.at(handle).mesh);
    }

    virtual void wheelEvent(QWheelEvent* event) override {
        if (event->modifiers() & Qt::CTRL) {
            float y0 = std::atan(0.5 * fov_);
            fov_ += 0.0004 * event->angleDelta().y();
            fov_ = std::max(0.01f, std::min(fov_, float(M_PI) / 2.f - 0.01f));
            std::cout << "Setting fov = " << fov_ * 180.f / M_PI << std::endl;
            float y1 = std::atan(0.5 * fov_);
            camera_.zoom(y0 / y1);
            updateCamera();
        } else if (event->modifiers() & Qt::ALT) {
            // pointSize_ += 0.01 * event->angleDelta().x();
            pointSize_ = std::max(std::min(pointSize_ + 0.005f * event->angleDelta().x(), 16.f), 1.f);

            glPointSize(int(pointSize_));
            std::cout << "Point size = " << pointSize_ << std::endl;
        } else {
            camera_.zoom(1 + 0.0004 * event->angleDelta().y());
        }
        update();
    }

    virtual void mousePressEvent(QMouseEvent* event) override;

    virtual void mouseDoubleClickEvent(QMouseEvent* event) override;

    virtual void mouseMoveEvent(QMouseEvent* ev) override;

private:
    void updateCamera();

    template <typename MeshFunc>
    void meshOperation(const MeshFunc& meshFunc);
};
