#include "openglwidget.h"
#include "ambient.h"
#include "pvl/QuadricDecimator.hpp"
#include "pvl/Refinement.hpp"
#include "pvl/Simplification.hpp"
#include "pvl/TriangleMesh.hpp"
#include <QPainter>
#include <tbb/tbb.h>

#ifdef foreach
#undef foreach // every time a programmer defines a macro, god kills a kitten
#endif
#include <openvdb/openvdb.h>
#include <openvdb/tools/MeshToVolume.h>
#include <openvdb/tools/VolumeToMesh.h>

void OpenGLWidget::resizeGL(const int width, const int height) {
    std::cout << "Resizing " << width << " " << height << std::endl;
    glViewport(0, 0, width, height);

    if (!meshes_.empty()) {
        float dist = Pvl::norm(camera_.eye() - camera_.target());
        std::cout << "Resizing GL, dist = " << dist << std::endl;
        PVL_ASSERT(dist > 0.0001);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(fov_ * 180.f / M_PI, float(width) / height, 0.001 * dist, 1000. * dist);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        updateCamera();
        //  updateLights(camera_);
        // glEnable(GL_LIGHTING);
        // glEnable(GL_LIGHT0);
    }
}

void OpenGLWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0, 0, 0, 0);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glShadeModel(GL_FLAT);
    // updateLights();
    glEnable(GL_LIGHTING);

#if 0
    GLfloat ambient[] = { 0, 0, 0, 1 };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);

    GLfloat pos1[] = { -2.e4f, 2.e4f, 4.e4f };
    GLfloat pos2[] = { 5.e4f, 0.f, 1.e4f };
    GLfloat diffuse1[] = { 1.f, 0.95f, 0.9f, 1.0 };
    GLfloat diffuse2[] = { 0.4f, 0.45f, 0.5f, 1.f };
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, pos1);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse1);
    glEnable(GL_LIGHT1);
    glLightfv(GL_LIGHT1, GL_POSITION, pos2);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse2);
#else

    // GLfloat ambient[] = { 0, 0, 0, 1 };
    // glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);

    // GLfloat pos1[] = { -2.e4f, 2.e4f, 4.e4f };
    // GLfloat pos2[] = { 5.e4f, 0.f, 1.e4f };
    GLfloat diffuse1[] = { 0.9f, 0.9f, 0.9f, 1.0 };
    // GLfloat diffuse2[] = { 0.4f, 0.45f, 0.5f, 1.f };
    glEnable(GL_LIGHT0);
    // glLightfv(GL_LIGHT0, GL_POSITION, pos1);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse1);
#endif

    // glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
    // glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    // glEnable(GL_LIGHT1);
    //  glEnable(GL_AUTO_NORMAL);
    // glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_DIFFUSE);
    // glColorMaterial(GL_FRONT, GL_AMBIENT);
    // glColorMaterial(GL_FRONT, GL_SPECULAR);

    // updateLights(1000.);
    // GLfloat specular[] = { 1., 1., 1., 1.0 };
    // glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

    // glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
    // glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    // glMaterialf(GL_FRONT, GL_SHININESS, 128);
    // GLfloat ambient[] = { 0.01, 0.01, 0.01, 1.0 };
    // glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

    glPointSize(pointSize_);
}

void OpenGLWidget::paintGL() {
    // std::cout << "Called paintGL" << std::endl;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glClearColor(0, 0.1, 0.3, 1);
    glClearColor(0.5, 0.5, 0.6, 1);
    glEnable(GL_DEPTH_TEST); // gets disabled by text rendering
    if (meshes_.empty()) {
        glFlush();
        return;
    }
    // updateLights(camera_);

    //    glLoadIdentity();
    /*GLfloat pos[] = { 5000., 0, 100. };
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    GLfloat diffuse[] = { 1., 1., 1., 1.0 };
   glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);*/
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float dist = Pvl::norm(camera_.eye() - camera_.target());

    gluPerspective(fov_ * 180.f / M_PI, float(width()) / height(), 0.001 * dist, 1000. * dist);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // glMatrixMode(GL_MODELVIEW);
    // glLoadIdentity();
    {
        Pvl::Vec3f target = camera_.target();
        Pvl::Vec3f eye = camera_.eye();
        Pvl::Vec3f up = camera_.up();
        gluLookAt(eye[0], eye[1], eye[2], target[0], target[1], target[2], up[0], up[1], up[2]);
    }
    //  updateLights(camera_);


    glColor3f(0.75, 0.75, 0.75);
    // glColor3f(0, 0, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPointSize(pointSize_);
    for (auto& p : meshes_) {
        if (!p.second.enabled) {
            continue;
        }

        if (vbos_) {
            glBindBuffer(GL_ARRAY_BUFFER, p.second.vbo);
        }

        if (p.second.hasNormals()) {
            glEnableClientState(GL_NORMAL_ARRAY);
        } else {
            glDisable(GL_LIGHTING);
        }
        if (p.second.hasColors()) {
            glEnableClientState(GL_COLOR_ARRAY);
            glDisable(GL_LIGHTING);
            glShadeModel(GL_SMOOTH);
        }
        glEnableClientState(GL_VERTEX_ARRAY);

        if (!vbos_) {
            glVertexPointer(3, GL_FLOAT, 0, p.second.vis.vertices.data());
            if (p.second.hasNormals()) {
                glNormalPointer(GL_FLOAT, 0, p.second.vis.normals.data());
            }
            if (p.second.hasColors()) {
                glColorPointer(3, GL_UNSIGNED_BYTE, 0, p.second.vis.colors.data());
            }
        } else {
            glVertexPointer(3, GL_FLOAT, 0, (void*)0);
            if (p.second.hasNormals()) {
                glNormalPointer(GL_FLOAT, 0, (void*)(p.second.vis.vertices.size() * sizeof(float)));
            }
            if (p.second.hasColors()) {
                glColorPointer(3,
                    GL_UNSIGNED_BYTE,
                    0,
                    (void*)((p.second.vis.vertices.size() + p.second.vis.normals.size()) * sizeof(float)));
            }
        }

        if (p.second.pointCloud()) {
            glDrawArrays(GL_POINTS, 0, p.second.vis.vertices.size() / 3);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, p.second.vis.vertices.size() / 3);
        }

        glDisableClientState(GL_VERTEX_ARRAY);
        if (p.second.hasColors()) {
            glDisableClientState(GL_COLOR_ARRAY);
            glEnable(GL_LIGHTING);
            glShadeModel(GL_FLAT);
        }
        if (p.second.hasNormals()) {
            glDisableClientState(GL_NORMAL_ARRAY);
        } else {
            glEnable(GL_LIGHTING);
        }

        if (vbos_) {
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }
    // glDisableClientState(GL_VERTEX_ARRAY);
    // glDisableClientState(GL_NORMAL_ARRAY);

    if (wireframe_ || dots_) {
        Pvl::Vec3f delta = -camera_.direction() * 5.e-4f * dist;
        glTranslatef(delta[0], delta[1], delta[2]);
        glColor3f(0, 0, 0);
        glPolygonMode(GL_FRONT_AND_BACK, wireframe_ ? GL_LINE : GL_POINT);

        //  glEnableClientState(GL_NORMAL_ARRAY);
        // glEnableClientState(GL_VERTEX_ARRAY);

        /*        for (auto& p : meshes_) {
                    if (!p.second.enabled) {
                        continue;
                    }
                    glNormalPointer(GL_FLOAT, 0, p.second.vis.normals.data());
                    glVertexPointer(3, GL_FLOAT, 0, p.second.vis.vertices.data());
                    glDrawArrays(GL_TRIANGLES, 0, p.second.vis.vertices.size() / 3);
                }*/

        for (auto& p : meshes_) {
            if (!p.second.enabled || p.second.pointCloud()) {
                continue;
            }

            glBindBuffer(GL_ARRAY_BUFFER, p.second.vbo);

            glEnableClientState(GL_NORMAL_ARRAY);
            glEnableClientState(GL_VERTEX_ARRAY);

            /* glVertexPointer(3, GL_FLOAT, 0, p.second.vis.vertices.data());
             glNormalPointer(GL_FLOAT, 0, p.second.vis.normals.data());
             glDrawArrays(GL_TRIANGLES, 0, p.second.vis.vertices.size() / 3);*/
            glVertexPointer(3, GL_FLOAT, 0, (void*)0);
            glNormalPointer(GL_FLOAT, 0, (void*)(p.second.vis.vertices.size() * sizeof(float)));
            glDrawArrays(GL_TRIANGLES, 0, p.second.vis.vertices.size() / 3);

            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_NORMAL_ARRAY);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }

    glColor3f(0, 0, 0);
    Pvl::Box3f bbox;
    for (const auto& p : meshes_) {
        if (p.second.enabled) {
            bbox.extend(p.second.box);
        }
    }
    float x1 = bbox.lower()[0] - 0.5f * grid_;
    float x2 = x1 + ceil(bbox.size()[0] / grid_ + 0.5f) * grid_;
    float y1 = bbox.lower()[1] - 0.5f * grid_;
    float y2 = y1 + ceil(bbox.size()[1] / grid_ + 0.5f) * grid_;
    float z0 = bbox.center()[2];
    if (showGrid_) {
        glBegin(GL_LINES);
        for (float x = x1; x <= x2; x += grid_) {
            glVertex3f(x, y1, z0);
            glVertex3f(x, y2, z0);
        }
        for (float y = y1; y <= y2; y += grid_) {
            glVertex3f(x1, y, z0);
            glVertex3f(x2, y, z0);
        }
        glEnd();
    }


    QPainter painter(this);
    painter.setPen(Qt::black);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    if (showGrid_) {
        painter.setFont(QFont("Helvetica", 8));
        for (float x = x1; x <= x2; x += grid_) {
            if (Pvl::Optional<Pvl::Vec2f> proj = camera_.unproject(Pvl::Vec3f(x, y1 - 0.1 * grid_, z0))) {
                painter.drawText(proj.value()[0], proj.value()[1], QString::number(x - x1));
            }
        }
        for (float y = y1; y <= y2; y += grid_) {
            if (Pvl::Optional<Pvl::Vec2f> proj = camera_.unproject(Pvl::Vec3f(x1 - 0.16 * grid_, y, z0))) {
                painter.drawText(proj.value()[0], proj.value()[1], QString::number(y - y1));
            }
        }
    }

    painter.setFont(QFont("Helvetica", 10));

    std::size_t numVertex = 0, numFaces = 0;
    for (const auto& p : meshes_) {
        if (p.second.enabled) {
            numVertex += p.second.mesh.vertices.size();
            numFaces += p.second.mesh.faces.size();
        }
    }
    painter.drawText(30, height() - 50, "Vertices:");
    painter.drawText(100, height() - 50, QString("%L1").arg(numVertex));
    painter.drawText(30, height() - 30, "Faces:");
    painter.drawText(100, height() - 30, QString("%L1").arg(numFaces));
    painter.end();


    glFlush();
}

void OpenGLWidget::view(const void* handle, Mesh&& mesh) {
    bool firstMesh = meshes_.empty();
    bool updateOnly = meshes_.find(handle) != meshes_.end();
    MeshData& data = meshes_[handle];
    data.mesh = std::move(mesh);
    data.vis = {};

    if (firstMesh) {
        srs_ = data.mesh.srs;
    }

    SrsConv conv(data.mesh.srs, srs_);
    if (data.mesh.faces.empty()) {
        // point cloud
        bool hasNormals = !data.mesh.normals.empty();
        bool hasColors = !data.mesh.colors.empty();
        data.vis.vertices.reserve(data.mesh.vertices.size() * 3);
        if (hasNormals) {
            data.vis.normals.reserve(data.mesh.vertices.size() * 3);
        }
        if (hasColors) {
            data.vis.colors.reserve(data.mesh.vertices.size() * 3);
        }
        for (std::size_t vi = 0; vi < data.mesh.vertices.size(); ++vi) {
            Pvl::Vec3f vertex = conv(data.mesh.vertices[vi]);
            data.vis.vertices.push_back(vertex[0]);
            data.vis.vertices.push_back(vertex[1]);
            data.vis.vertices.push_back(vertex[2]);

            if (hasNormals) {
                const Pvl::Vec3f normal = Pvl::normalize(data.mesh.normals[vi]);
                data.vis.normals.push_back(normal[0]);
                data.vis.normals.push_back(normal[1]);
                data.vis.normals.push_back(normal[2]);
            }
            if (hasColors) {
                const Color& c = data.mesh.colors[vi];
                data.vis.colors.push_back(c[0]);
                data.vis.colors.push_back(c[1]);
                data.vis.colors.push_back(c[2]);
            }
        }
    } else {
        // mesh
        bool hasColors = !data.mesh.colors.empty();
        data.vis.vertices.reserve(data.mesh.faces.size() * 9);
        data.vis.normals.reserve(data.mesh.faces.size() * 9);
        if (hasColors) {
            data.vis.colors.reserve(data.mesh.faces.size() * 9);
        }
        for (std::size_t fi = 0; fi < data.mesh.faces.size(); ++fi) {
            Pvl::Vec3f normal = data.mesh.normal(fi);
            for (int i = 0; i < 3; ++i) {
                Pvl::Vec3f vertex = conv(data.mesh.vertices[data.mesh.faces[fi][i]]);
                data.vis.vertices.push_back(vertex[0]);
                data.vis.vertices.push_back(vertex[1]);
                data.vis.vertices.push_back(vertex[2]);

                data.vis.normals.push_back(normal[0]);
                data.vis.normals.push_back(normal[1]);
                data.vis.normals.push_back(normal[2]);

                if (hasColors) {
                    Color c = data.mesh.colors[data.mesh.faces[fi][i]];
                    data.vis.colors.push_back(c[0]);
                    data.vis.colors.push_back(c[1]);
                    data.vis.colors.push_back(c[2]);
                }
            }
        }
    }

    if (vbos_) {
        if (!updateOnly) {
            glGenBuffers(1, &data.vbo);
        }
        glBindBuffer(GL_ARRAY_BUFFER, data.vbo);
        glBufferData(GL_ARRAY_BUFFER,
            (data.vis.vertices.size() + data.vis.normals.size()) * sizeof(float) +
                data.vis.colors.size() * sizeof(uint8_t),
            0,
            GL_STATIC_DRAW);
        glBufferSubData(
            GL_ARRAY_BUFFER, 0, data.vis.vertices.size() * sizeof(float), data.vis.vertices.data());
        glBufferSubData(GL_ARRAY_BUFFER,
            data.vis.vertices.size() * sizeof(float),
            data.vis.normals.size() * sizeof(float),
            data.vis.normals.data());
        glBufferSubData(GL_ARRAY_BUFFER,
            data.vis.vertices.size() * sizeof(float) + data.vis.normals.size() * sizeof(float),
            data.vis.colors.size() * sizeof(uint8_t),
            data.vis.colors.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    data.box = Pvl::Box3f{};
    for (const Pvl::Vec3f& p : data.mesh.vertices) {
        data.box.extend(p);
    }
    std::cout << "Mesh has extents " << data.box.lower()[0] << "," << data.box.lower()[1] << ":"
              << data.box.upper()[0] << "," << data.box.upper()[1] << std::endl;

    // std::cout << "First mesh = " << firstMesh << std::endl;
    if (firstMesh) {
        Pvl::Vec3f center = data.box.center();
        float scale = std::max(data.box.size()[0], data.box.size()[1]);
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
    }
    update();
}

void OpenGLWidget::updateCamera() {
    Pvl::Vec3f eye = camera_.eye();
    Pvl::Vec3f target = camera_.target();
    Pvl::Vec3f up = camera_.up();
    camera_ = Camera(eye, target, up, fov_, Pvl::Vec2i(width(), height()));
}


void OpenGLWidget::mousePressEvent(QMouseEvent* event) {
    if (meshes_.empty()) {
        return;
    }
    mouse_.pos0 = event->pos();
    mouse_.state = camera_;
    if (event->button() == Qt::RightButton) {
        mouse_.ab.initialize(Pvl::Vec2i(width(), height()), camera_.matrix());
        mouse_.ab.click(Pvl::Vec2i(mouse_.pos0.x(), mouse_.pos0.y()));
    }
}

void OpenGLWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    mouse_.pos0 = event->pos();

    std::cout << "Clicked at " << mouse_.pos0.x() << "," << mouse_.pos0.y() << std::endl;
    Ray ray = camera_.project(Pvl::Vec2f(mouse_.pos0.x(), mouse_.pos0.y()));

    float t_min = std::numeric_limits<float>::max();
    // selected = Pvl::NONE;
    tbb::mutex mutex;
    for (const auto& p : meshes_) {
        if (p.second.pointCloud()) {
            /*float dist = Pvl::norm(camera_.eye() - camera_.target());
            float radius = 1.f; // * pointSize_ * dist / std::tan(fov_ / 2.f) / height();
            tbb::parallel_for<std::size_t>(0, p.second.mesh.vertices.size(), [&](std::size_t vi) {
                const Pvl::Vec3f& point = p.second.mesh.vertices[vi];
                float t;
                if (intersection(ray, point, radius, t) && t > 0 && t < t_min) {
                    mutex.lock();
                    t_min = t;
                    mutex.unlock();
                }
            });*/
            if (ray.dir[2] < 0) {
                t_min = -ray.origin[2] / ray.dir[2];
            }
        } else {
            tbb::parallel_for<std::size_t>(0, p.second.mesh.faces.size(), [&](std::size_t fi) {
                Triangle tri;
                for (int i = 0; i < 3; ++i) {
                    tri[i] = p.second.mesh.vertices[p.second.mesh.faces[fi][i]];
                }
                float t;
                if (intersection(ray, tri, t) && t > 0 && t < t_min) {
                    mutex.lock();
                    t_min = t;
                    mutex.unlock();
                }
            });
        }
    }

    if (t_min != std::numeric_limits<float>::max()) {
        Pvl::Vec3f target = ray.origin + ray.dir * t_min;
        camera_.lookAt(target);
    }
    // selected = Pvl::NONE;
    update();
}

void OpenGLWidget::mouseMoveEvent(QMouseEvent* ev) {
    if (meshes_.empty()) {
        return;
    }
    camera_ = mouse_.state;
    if (ev->buttons() & Qt::RightButton) {
        QPoint p = ev->pos();
        Pvl::Mat33f m = mouse_.ab.drag(Pvl::Vec2i(p.x(), p.y()));
        camera_.transform(m);
    } else {
        QPoint dp = ev->pos() - mouse_.pos0;
        camera_.pan(Pvl::Vec2i(dp.x(), dp.y()));
    }

    update();
}

template <typename MeshFunc>
void OpenGLWidget::meshOperation(const MeshFunc& meshFunc) {
    std::vector<std::pair<const void*, MeshData*>> meshData;
    // cannot erase from meshes_ while iterating, so add it to a vector
    for (auto& p : meshes_) {
        if (!p.second.pointCloud()) {
            meshData.emplace_back(p.first, &p.second);
        }
    }
    // also backup camera
    auto cameraState = camera_;

    for (const auto& p : meshData) {
        const void* handle = p.first;
        Mesh mesh = std::move(p.second->mesh);
        deleteMesh(handle);
        Pvl::TriangleMesh<Pvl::Vec3f> trimesh;
        for (const Pvl::Vec3f& p : mesh.vertices) {
            trimesh.addVertex();
            trimesh.points.push_back(p);
        }
        /// \todo external point storage?
        mesh.vertices = {}; // free memory
        for (const Mesh::Face& f : mesh.faces) {
            trimesh.addFace(Pvl::VertexHandle(f[0]), Pvl::VertexHandle(f[1]), Pvl::VertexHandle(f[2]));
        }
        mesh.faces = {};

        meshFunc(trimesh);

        for (const Pvl::Vec3f& p : trimesh.points) {
            mesh.vertices.push_back(p);
        }
        for (Pvl::FaceHandle fh : trimesh.faceRange()) {
            if (!trimesh.valid(fh)) {
                continue;
            }
            auto face = trimesh.faceVertices(fh);
            mesh.faces.push_back(Mesh::Face{ face[0].index(), face[1].index(), face[2].index() });
        }
        view(handle, std::move(mesh));
    }

    camera_ = cameraState;
    update();
}

void OpenGLWidget::laplacianSmooth() {
    meshOperation([](Pvl::TriangleMesh<Pvl::Vec3f>& mesh) { Pvl::laplacianSmoothing(mesh, true, 0.f); });
}

void OpenGLWidget::simplify() {
    meshOperation([](Pvl::TriangleMesh<Pvl::Vec3f>& mesh) {
        // decimate to 1/4 faces
        Pvl::PreventFaceFoldDecorator<Pvl::QuadricDecimator<Pvl::TriangleMesh<Pvl::Vec3f>>> decimator(mesh);
        Pvl::simplify(mesh, decimator, Pvl::FaceCountStop(3 * mesh.numFaces() / 4));
    });
}

namespace {
class MeshAdapter {
    Mesh& mesh_;
    openvdb::math::Transform::Ptr tr_;


public:
    MeshAdapter(Mesh& mesh, openvdb::math::Transform::Ptr tr)
        : mesh_(mesh)
        , tr_(tr) {}

    size_t polygonCount() const {
        return mesh_.faces.size();
    }
    size_t pointCount() const {
        return mesh_.vertices.size();
    }
    size_t vertexCount(size_t) const {
        return 3;
    }

    void getIndexSpacePoint(size_t n, size_t v, openvdb::Vec3d& pos) const {
        const Pvl::Vec3f& p = mesh_.vertices[mesh_.faces[n][v]];
        pos = openvdb::Vec3d(p[0], p[1], p[2]);
        pos = tr_->worldToIndex(pos);
    }
};
} // namespace


void repairMesh(Mesh& mesh) {
    openvdb::initialize();

    Pvl::Box3f box;
    for (const Pvl::Vec3f& p : mesh.vertices) {
        box.extend(p);
    }
    float size = std::max({ box.size()[0], box.size()[1], box.size()[2] });
    openvdb::math::Transform::Ptr tr = openvdb::math::Transform::createLinearTransform(0.000666 * size);
    MeshAdapter adapter(mesh, tr);

    std::cout << "converting to volume" << std::endl;
    openvdb::FloatGrid::Ptr grid = openvdb::tools::meshToVolume<openvdb::FloatGrid>(adapter, *tr, 1., 1.);
    std::cout << "Created volume with " << grid->activeVoxelCount() << " active voxels" << std::endl;
    std::cout << "converting to mesh" << std::endl;
    std::vector<openvdb::Vec3s> points;
    std::vector<openvdb::Vec3I> triangles;
    std::vector<openvdb::Vec4I> quads;
    openvdb::tools::volumeToMesh(*grid, points, triangles, quads);

    mesh = {};
    for (auto p : points) {
        mesh.vertices.emplace_back(p.x(), p.y(), p.z());
    }
    for (auto& f : triangles) {
        mesh.faces.emplace_back(Mesh::Face{ f[0], f[2], f[1] });
    }
    for (auto& f : quads) {
        mesh.faces.emplace_back(Mesh::Face{ f[0], f[2], f[1] });
        mesh.faces.emplace_back(Mesh::Face{ f[0], f[3], f[2] });
    }

    openvdb::uninitialize();
}

void OpenGLWidget::repair() {
    std::vector<std::pair<const void*, MeshData*>> meshData;
    // cannot erase from meshes_ while iterating, so add it to a vector
    for (auto& p : meshes_) {
        if (!p.second.pointCloud()) {
            meshData.emplace_back(p.first, &p.second);
        }
    }
    // also backup camera
    auto cameraState = camera_;

    for (const auto& p : meshData) {
        const void* handle = p.first;
        Mesh mesh = std::move(p.second->mesh);
        deleteMesh(handle);
        repairMesh(mesh);
        view(handle, std::move(mesh));
    }

    camera_ = cameraState;
    update();
}

void OpenGLWidget::computeAmbientOcclusion(std::function<bool(float)> progress) {
    // also backup camera
    auto cameraState = camera_;

    /// \todo no need to delete mesh, only updates
    for (auto& p : meshes_) {
        const void* handle = p.first;
        if (p.second.pointCloud() || !p.second.vis.colors.empty()) {
            // pc or already computed
            p.second.flat = false;
            continue;
        }
        Mesh mesh = std::move(p.second.mesh);
        //  deleteMesh(handle);
        ::computeAmbientOcclusion(mesh, progress);
        p.second.flat = false;
        view(handle, std::move(mesh));
    }

    camera_ = cameraState;
    update();
}
