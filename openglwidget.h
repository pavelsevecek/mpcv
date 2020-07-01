#pragma once

#include "camera.h"
#include "pvl/Box.hpp"
#include "pvl/Optional.hpp"
//#include "pvl/TriangleMesh.hpp"
#include "mesh.h"
#include "quaternion.h"
#include <GL/glu.h>
#include <QMouseEvent>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QWheelEvent>
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

class OpenGLWidget : public QOpenGLWidget, public QOpenGLFunctions {
    Q_OBJECT

    struct MeshData {
        Mesh mesh;
        bool enabled = true;

        struct {
            std::vector<float> vertices;
            std::vector<float> normals;
        } vis;

        GLuint vbo;

        bool pointCloud() const {
            return mesh.faces.empty();
        }
        bool hasNormals() const {
            return !pointCloud() || !mesh.normals.empty();
        }
    };
    Pvl::Optional<Triangle> selected;

    Camera camera_;
    float fov_ = M_PI / 4.f;
    float pointSize_ = 2;

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

    void view(const void* handle, Mesh&& mesh);

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

    /*void orientation(const bool outward) {
        std::cout << "Changing orientation to " << outward << std::endl;
        glFrontFace(outward ? GL_CCW : GL_CW);
        update();
    }*/

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
};
