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
            // std::vector<int> indices;
        } vis;

        GLuint vbo;
    };
    Pvl::Optional<Triangle> selected;

    Camera camera_;

    std::map<const void*, MeshData> meshes_;
    bool wireframe_ = false;
    bool dots_ = false;
    bool vbos = false;

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


    virtual void wheelEvent(QWheelEvent* event) override {
        std::cout << "Wheel event!" << std::endl;
        camera_.zoom(1 + 0.0004 * event->angleDelta().y());
        update();
        // updateCamera();
    }

    virtual void mousePressEvent(QMouseEvent* event) override;

    virtual void mouseDoubleClickEvent(QMouseEvent* event) override;

    virtual void mouseMoveEvent(QMouseEvent* ev) override;

private:
    void updateCamera();
};
