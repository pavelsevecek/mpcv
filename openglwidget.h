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

class OpenGLWidget : public QOpenGLWidget, public QOpenGLFunctions {
    Q_OBJECT

    struct MeshData {
        Mpcv::TexturedMesh mesh;
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
        bool hasAo() const {
            return !mesh.ao.empty();
        }
        bool hasTexture() const {
            // point cloud cannot have texture
            return !pointCloud() && !mesh.uv.empty();
        }
    };
    // Pvl::Optional<Triangle> selected;

    Mpcv::Camera camera_;
    float fov_ = M_PI / 4.f;
    float pointSize_ = 2.f;
    float pointStride_ = 1.f;
    float grid_ = 1.f;
    bool showGrid_ = false;

    bool enableAo_ = false;
    bool enableTextures_ = true;

    std::map<const void*, MeshData> meshes_;
    bool wireframe_ = false;
    bool dots_ = false;
    bool vbos_ = true;

    struct {
        QPoint pos0;
        Mpcv::ArcBall ab;
        Mpcv::Camera state;
    } mouse_;

    Pvl::Vec3f sunDir_ = Pvl::normalize(Pvl::Vec3f(1.f, 1.f, 4.f));

public:
    OpenGLWidget(QWidget* parent)
        : QOpenGLWidget(parent) {}

    virtual void initializeGL() override;

    virtual void resizeGL(const int width, const int height) override;

    virtual void paintGL() override;

    void view(const void* handle, Mpcv::TexturedMesh&& mesh);

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

    void deleteMesh(const void* handle);

    void laplacianSmooth();

    void simplify();

    void repair();

    void estimateNormals(std::function<bool(float)> progress);

    void computeAmbientOcclusion(std::function<bool(float)> progress);

    void enableAo(bool on) {
        enableAo_ = on;
        if (on) {
            enableTextures_ = false;
        }
        update();
    }

    void enableTextures(bool on) {
        enableTextures_ = on;
        if (on) {
            enableAo_ = false;
        }
        update();
    }

    void grid(bool on) {
        showGrid_ = on;
        update();
    }

    void resetCamera();

    void resetCamera(const Mpcv::Srs& srs);

    void screenshot(const QString& file);

    void saveAsMesh(const QString& file, const std::vector<const void*>& handles);

    void setSunDir(const Pvl::Vec3f& dir) {
        sunDir_ = dir;
    }

    void renderView();

    virtual void wheelEvent(QWheelEvent* event) override;

    virtual void mousePressEvent(QMouseEvent* event) override;

    virtual void mouseDoubleClickEvent(QMouseEvent* event) override;

    virtual void mouseMoveEvent(QMouseEvent* ev) override;

private:
    void updateCamera();

    template <typename MeshFunc>
    void meshOperation(const MeshFunc& meshFunc);
};
