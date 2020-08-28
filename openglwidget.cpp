#include "openglwidget.h"
#include "framebuffer.h"
#include "pvl/CloudUtils.hpp"
#include "pvl/QuadricDecimator.hpp"
#include "pvl/Refinement.hpp"
#include "pvl/Simplification.hpp"
#include "pvl/TriangleMesh.hpp"
#include "renderer.h"
#include <QPainter>
#include <sstream>
#include <tbb/tbb.h>

#ifdef HAS_OPENVDB
#ifdef foreach
#undef foreach // every time a programmer defines a macro, god kills a kitten
#endif
#include <openvdb/openvdb.h>
#include <openvdb/tools/MeshToVolume.h>
#include <openvdb/tools/VolumeToMesh.h>
#endif

using namespace Mpcv;

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
    glEnable(GL_TEXTURE_2D);
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
    // glEnable(GL_TEXTURE_2D);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPointSize(pointSize_);
    for (const auto& p : meshes_) {
        const MeshData& mesh = p.second;
        if (!mesh.enabled) {
            continue;
        }

        if (vbos_) {
            glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
        }

        bool useNormals = mesh.hasNormals();
        bool useColors;
        if (mesh.pointCloud()) {
            // for point clouds, point colors are considered a texture here
            useColors = mesh.hasColors() && enableTextures_;
        } else {
            useColors = mesh.hasColors() || (enableAo_ && mesh.hasAo());
        }
        bool useClasses = enableClasses_ && mesh.hasClasses();
        bool useTexture = enableTextures_ && mesh.hasTexture();

        if (useColors || useTexture || !useNormals) {
            glDisable(GL_LIGHTING);
        }
        if (useNormals) {
            glEnableClientState(GL_NORMAL_ARRAY);
        }
        if (useColors) {
            glEnableClientState(GL_COLOR_ARRAY);
            glShadeModel(GL_SMOOTH); // for AO
        }
        if (useClasses) {
            glEnableClientState(GL_COLOR_ARRAY);
        }
        if (useTexture) {
            glBindTexture(GL_TEXTURE_2D, mesh.texture);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        }
        glEnableClientState(GL_VERTEX_ARRAY);

        int numVert = mesh.vis.vertices.size();
        int numNorm = mesh.vis.normals.size();
        int numClr = mesh.vis.vertexColors.size();
        int numCls = mesh.vis.classColors.size();
        int stride = mesh.pointCloud() ? int(pointStride_) : 0;

        if (!vbos_) {
            glVertexPointer(3, GL_FLOAT, stride * 3 * sizeof(float), mesh.vis.vertices.data());
            if (useNormals) {
                glNormalPointer(GL_FLOAT, stride * 3 * sizeof(float), mesh.vis.normals.data());
            }
            if (useClasses && !enableAo_) {
                glColorPointer(
                    3, GL_UNSIGNED_BYTE, stride * 3 * sizeof(uint8_t), mesh.vis.classColors.data());
            } else if (useColors) {
                glColorPointer(
                    3, GL_UNSIGNED_BYTE, stride * 3 * sizeof(uint8_t), mesh.vis.vertexColors.data());
            }
            if (useTexture) {
                // never used by pc
                glTexCoordPointer(2, GL_FLOAT, 0, mesh.vis.uv.data());
            }
        } else {
            glVertexPointer(3, GL_FLOAT, stride * 3 * sizeof(float), (void*)0);
            if (useNormals) {
                glNormalPointer(GL_FLOAT, stride * 3 * sizeof(float), (void*)(numVert * sizeof(float)));
            }
            if (useClasses && !enableAo_) {
                glColorPointer(3,
                    GL_UNSIGNED_BYTE,
                    stride * 3 * sizeof(uint8_t),
                    (void*)((numVert + numNorm) * sizeof(float) + numClr * sizeof(uint8_t)));
            } else if (useColors) {
                glColorPointer(3,
                    GL_UNSIGNED_BYTE,
                    stride * 3 * sizeof(uint8_t),
                    (void*)((numVert + numNorm) * sizeof(float)));
            }
            if (useTexture) {
                glTexCoordPointer(2,
                    GL_FLOAT,
                    0,
                    (void*)((numVert + numNorm) * sizeof(float) + (numClr + numCls) * sizeof(uint8_t)));
            }
        }

        if (mesh.pointCloud()) {
            glDrawArrays(GL_POINTS, 0, mesh.vis.vertices.size() / 3 / stride);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, mesh.vis.vertices.size() / 3);
        }

        glDisableClientState(GL_VERTEX_ARRAY);
        if (useTexture) {
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        if (useColors) {
            glDisableClientState(GL_COLOR_ARRAY);
            glShadeModel(GL_FLAT);
        }
        if (useClasses) {
            glDisableClientState(GL_COLOR_ARRAY);
        }
        if (useNormals) {
            glDisableClientState(GL_NORMAL_ARRAY);
        }
        glEnable(GL_LIGHTING);

        if (vbos_) {
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }

    glDisable(GL_LIGHTING);
    if (wireframe_ || dots_) {
        Pvl::Vec3f delta = -camera_.direction() * 5.e-4f * dist;
        glTranslatef(delta[0], delta[1], delta[2]);
        glColor3f(0, 0, 0);
        glPolygonMode(GL_FRONT_AND_BACK, wireframe_ ? GL_LINE : GL_POINT);

        for (const auto& p : meshes_) {
            const MeshData& mesh = p.second;
            if (!mesh.enabled || mesh.pointCloud()) {
                continue;
            }

            glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(3, GL_FLOAT, 0, (void*)0);
            glDrawArrays(GL_TRIANGLES, 0, mesh.vis.vertices.size() / 3);
            glDisableClientState(GL_VERTEX_ARRAY);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glColor3f(0, 0, 0);
    Pvl::Box3f bbox;
    std::vector<std::pair<Pvl::Vec3f, QString>> textToRender;
    QFont font("Helvetica", 8);
    QFontMetrics fontMetrics(font);
    for (const auto& p : meshes_) {
        const MeshData& mesh = p.second;
        if (!mesh.enabled) {
            continue;
        }
        SrsConv conv(mesh.mesh.srs, camera_.srs());
        Pvl::Box3f meshBox(conv(mesh.box.lower()), conv(mesh.box.upper()));
        bbox.extend(meshBox);

        if (bboxes_) {
            float x1 = meshBox.lower()[0];
            float y1 = meshBox.lower()[1];
            float z1 = meshBox.lower()[2];
            float x2 = meshBox.upper()[0];
            float y2 = meshBox.upper()[1];
            float z2 = meshBox.upper()[2];
            glBegin(GL_LINE_STRIP);
            glVertex3f(x1, y1, z1);
            glVertex3f(x2, y1, z1);
            glVertex3f(x2, y2, z1);
            glVertex3f(x1, y2, z1);
            glVertex3f(x1, y1, z1);
            glVertex3f(x1, y1, z2);
            glVertex3f(x2, y1, z2);
            glVertex3f(x2, y2, z2);
            glVertex3f(x1, y2, z2);
            glVertex3f(x1, y1, z2);
            glEnd();
            glBegin(GL_LINES);
            glVertex3f(x2, y1, z1);
            glVertex3f(x2, y1, z2);
            glVertex3f(x2, y2, z1);
            glVertex3f(x2, y2, z2);
            glVertex3f(x1, y2, z1);
            glVertex3f(x1, y2, z2);
            glEnd();
            QString basename = QString::fromStdString(mesh.basename);
            textToRender.emplace_back(meshBox.center(), basename);
        }
    }
    if (showGrid_) {
        float x1 = bbox.lower()[0] - 0.5f * grid_;
        float x2 = x1 + ceil(bbox.size()[0] / grid_ + 0.5f) * grid_;
        float y1 = bbox.lower()[1] - 0.5f * grid_;
        float y2 = y1 + ceil(bbox.size()[1] / grid_ + 0.5f) * grid_;
        float z0 = bbox.center()[2];
        glBegin(GL_LINES);
        for (float x = x1; x <= x2; x += grid_) {
            glVertex3f(x, y1, z0);
            glVertex3f(x, y2, z0);
            textToRender.emplace_back(Pvl::Vec3f(x, y1 - 0.1 * grid_, z0), QString::number(x - x1));
        }
        for (float y = y1; y <= y2; y += grid_) {
            glVertex3f(x1, y, z0);
            glVertex3f(x2, y, z0);
            textToRender.emplace_back(Pvl::Vec3f(x1 - 0.16 * grid_, y, z0), QString::number(y - y1));
        }
        glEnd();
    }

    QPainter painter(this);
    painter.setPen(Qt::black);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    // if (showGrid_) {
    painter.setFont(font);
    for (const auto& p : textToRender) {
        if (Pvl::Optional<Pvl::Vec2f> proj = camera_.unproject(p.first)) {
            QRect rect = fontMetrics.boundingRect(p.second);
            painter.drawText(
                proj.value()[0] - rect.width() / 2, proj.value()[1] + rect.height() / 2, p.second);
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
    glEnable(GL_LIGHTING);

    glFlush();
}

inline int toGlFormat(const ImageFormat& format) {
    switch (format) {
    case ImageFormat::GRAY:
        return GL_RED;
    case ImageFormat::RGB:
        return GL_RGB;
    case ImageFormat::BGR:
        return GL_BGR;
    case ImageFormat::RGBA:
        return GL_RGBA;
    case ImageFormat::BGRA:
        return GL_BGRA;
    default:
        throw std::runtime_error("Unknown image format " + std::to_string(int(format)));
    }
}

inline Color classToColor(const uint8_t c) {
    switch (c) {
    case 1:
        return Color(220, 220, 50); // interiors
    case 2:
        return Color(65, 140, 85); // tree
    case 3:
        return Color(150, 80, 10); // trunks
    case 4:
        return Color(20, 20, 20); // wire
    case 5:
        return Color(20, 20, 150); // cars
    default:
        return Color(190, 190, 190);
    }
}

void OpenGLWidget::view(const void* handle, std::string basename, TexturedMesh&& mesh) {
    bool firstMesh = meshes_.empty();
    bool updateOnly = meshes_.find(handle) != meshes_.end();
    MeshData& data = meshes_[handle];
    data.mesh = std::move(mesh);
    data.basename = basename;
    data.vis = {};

    Srs refSrs;
    if (firstMesh) {
        refSrs = data.mesh.srs;
    } else {
        refSrs = camera_.srs();
    }

    SrsConv conv(data.mesh.srs, refSrs);
    if (data.pointCloud()) {
        bool hasNormals = !data.mesh.normals.empty();
        bool hasColors = !data.mesh.colors.empty();
        bool hasClasses = !data.mesh.classes.empty();
        data.vis.vertices.reserve(data.mesh.vertices.size() * 3);
        if (hasNormals) {
            data.vis.normals.reserve(data.mesh.vertices.size() * 3);
        }
        if (hasColors) {
            data.vis.vertexColors.reserve(data.mesh.vertices.size() * 3);
        }
        if (hasClasses) {
            data.vis.classColors.reserve(data.mesh.vertices.size() * 3);
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
            if (hasClasses) {
                Color c = classToColor(data.mesh.classes[vi]);
                data.vis.classColors.push_back(c[0]);
                data.vis.classColors.push_back(c[1]);
                data.vis.classColors.push_back(c[2]);
            }
            if (hasColors) {
                const Color& c = data.mesh.colors[vi];
                data.vis.vertexColors.push_back(c[0]);
                data.vis.vertexColors.push_back(c[1]);
                data.vis.vertexColors.push_back(c[2]);
            }
        }
    } else {
        // mesh
        bool hasColors = !data.mesh.colors.empty();
        bool hasTexture = !data.mesh.uv.empty();
        bool hasAo = !data.mesh.ao.empty();
        bool hasClasses = !data.mesh.classes.empty();

        data.vis.vertices.reserve(data.mesh.faces.size() * 9);
        data.vis.normals.reserve(data.mesh.faces.size() * 9);
        if (hasAo || hasColors) {
            data.vis.vertexColors.reserve(data.mesh.faces.size() * 9);
        }
        if (hasClasses) {
            data.vis.classColors.reserve(data.mesh.faces.size() * 9);
        }
        if (hasTexture) {
            data.vis.uv.reserve(data.mesh.faces.size() * 6);
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

                if (hasAo) {
                    uint8_t ao = data.mesh.ao[3 * fi + i];
                    data.vis.vertexColors.push_back(ao);
                    data.vis.vertexColors.push_back(ao);
                    data.vis.vertexColors.push_back(ao);
                } else if (hasColors) {
                    Color c = data.mesh.colors[data.mesh.faces[fi][i]];
                    data.vis.vertexColors.push_back(c[0]);
                    data.vis.vertexColors.push_back(c[1]);
                    data.vis.vertexColors.push_back(c[2]);
                }
                if (hasClasses) {
                    Color c = classToColor(data.mesh.classes[data.mesh.faces[fi][i]]);
                    data.vis.classColors.push_back(c[0]);
                    data.vis.classColors.push_back(c[1]);
                    data.vis.classColors.push_back(c[2]);
                }
                if (hasTexture) {
                    Pvl::Vec2f uv = data.mesh.uv[data.mesh.texIds[fi][i]];
                    data.vis.uv.push_back(uv[0]);
                    data.vis.uv.push_back(1.f - uv[1]);
                }
            }
        }
        if (hasTexture && !updateOnly) {
            /// \todo allow editing texture?
            glGenTextures(1, &data.texture);
            glBindTexture(GL_TEXTURE_2D, data.texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            ITexture& tex = *data.mesh.texture;
            Pvl::Vec2i size = tex.size();
            int maxTextureSize;
            glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
            std::cout << "Max texture size = " << maxTextureSize << std::endl;
            int format = toGlFormat(tex.format());
            int internal = tex.format() == ImageFormat::GRAY ? GL_LUMINANCE : GL_RGB;
            glTexImage2D(
                GL_TEXTURE_2D, 0, internal, size[0], size[1], 0, format, GL_UNSIGNED_BYTE, tex.data());
            glGenerateMipmap(GL_TEXTURE_2D);
            data.mesh.texture.reset();
        }
    }
    if (vbos_) {
        if (!updateOnly) {
            glGenBuffers(1, &data.vbo);
        }
        int numVert = data.vis.vertices.size();
        int numNorm = data.vis.normals.size();
        int numClr = data.vis.vertexColors.size();
        int numCls = data.vis.classColors.size();
        int numTex = data.vis.uv.size();
        glBindBuffer(GL_ARRAY_BUFFER, data.vbo);
        glBufferData(GL_ARRAY_BUFFER,
            (numVert + numNorm + numTex) * sizeof(float) + (numClr + numCls) * sizeof(uint8_t),
            0,
            GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, numVert * sizeof(float), data.vis.vertices.data());
        glBufferSubData(
            GL_ARRAY_BUFFER, numVert * sizeof(float), numNorm * sizeof(float), data.vis.normals.data());
        glBufferSubData(GL_ARRAY_BUFFER,
            (numVert + numNorm) * sizeof(float),
            numClr * sizeof(uint8_t),
            data.vis.vertexColors.data());
        glBufferSubData(GL_ARRAY_BUFFER,
            (numVert + numNorm) * sizeof(float) + numClr * sizeof(uint8_t),
            numCls * sizeof(uint8_t),
            data.vis.classColors.data());
        glBufferSubData(GL_ARRAY_BUFFER,
            (numVert + numNorm) * sizeof(float) + (numClr + numCls) * sizeof(uint8_t),
            numTex * sizeof(float),
            data.vis.uv.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    data.box = Pvl::Box3f{};
    for (const Pvl::Vec3f& p : data.mesh.vertices) {
        data.box.extend(p);
    }
    std::cout << "Mesh has extents " << data.box.lower()[0] << "," << data.box.lower()[1] << ":"
              << data.box.upper()[0] << "," << data.box.upper()[1] << std::endl;


    if (firstMesh) {
        resetCamera(refSrs);
    }
}

void OpenGLWidget::deleteMesh(const void* handle) {
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

void OpenGLWidget::updateCamera() {
    Pvl::Vec3f eye = camera_.eye();
    Pvl::Vec3f target = camera_.target();
    Pvl::Vec3f up = camera_.up();
    Srs srs = camera_.srs();
    camera_ = Camera(eye, target, up, fov_, srs, Pvl::Vec2i(width(), height()));
}

void OpenGLWidget::resetCamera() {
    resetCamera(camera_.srs());
}

void OpenGLWidget::resetCamera(const Srs& srs) {
    // find first enabled
    for (const auto& p : meshes_) {
        const MeshData& mesh = p.second;
        if (!mesh.enabled) {
            continue;
        }

        SrsConv conv(mesh.mesh.srs, srs);
        Pvl::Vec3f center = conv(mesh.box.center());
        float scale = std::max(mesh.box.size()[0], mesh.box.size()[1]);
        float zoom = std::max(1.5f * scale, 0.001f);

        camera_ = Camera(center + Pvl::Vec3f(0, 0, zoom),
            center,
            Pvl::Vec3f(0, 1, 0),
            fov_,
            srs,
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

void OpenGLWidget::screenshot(const QString& file) {
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_FRONT);
    std::vector<uint8_t> pixels(width() * height() * 3);
    QRect rect = geometry();
    glReadPixels(
        rect.x(), rect.y(), rect.width(), rect.height(), GL_BGR_EXT, GL_UNSIGNED_BYTE, pixels.data());
    QImage image(pixels.data(), rect.width(), rect.height(), rect.width() * 3, QImage::Format_RGB888);
    QImageWriter writer(file);
    writer.write(std::move(image).mirrored().rgbSwapped());
}

void OpenGLWidget::saveAsMesh(const QString& file,
    const std::vector<const void*>& handles,
    std::function<bool(float)> progress) {
    std::ofstream ofs(file.toStdString());
    std::vector<const TexturedMesh*> meshes;
    for (auto handle : handles) {
        meshes.push_back(&meshes_[handle].mesh);
    }
    savePly(ofs, meshes, progress);
}

void OpenGLWidget::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::CTRL) {
        float y0 = std::atan(0.5 * fov_);
        fov_ += 0.0004 * event->angleDelta().y();
        fov_ = std::max(0.01f, std::min(fov_, float(M_PI) / 2.f - 0.01f));
        std::cout << "Setting fov = " << fov_ * 180.f / M_PI << std::endl;
        float y1 = std::atan(0.5 * fov_);
        camera_.zoom(y0 / y1);
        updateCamera();
        update();
    } else if (event->modifiers() & Qt::ALT) {
        // pointSize_ += 0.01 * event->angleDelta().x();
        pointSize_ = std::max(std::min(pointSize_ + 0.005f * event->angleDelta().x(), 16.f), 1.f);

        glPointSize(int(pointSize_));
        std::cout << "Point size = " << pointSize_ << std::endl;
        update();
    } else if (event->modifiers() & Qt::SHIFT) {
        int prevStride = int(pointStride_);
        pointStride_ =
            std::max(std::min(pointStride_ * (1.f + 0.003f * event->angleDelta().y()), 100.f), 1.f);
        std::cout << "Point stride = " << pointStride_ << std::endl;
        if (int(pointStride_) != prevStride) {
            update();
        }
    } else {
        camera_.zoom(1 + 0.0004 * event->angleDelta().y());
        update();
    }
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

    CameraRay ray = camera_.project(Pvl::Vec2f(mouse_.pos0.x(), mouse_.pos0.y()));

    const float t_inf = std::numeric_limits<float>::max();
    float mesh_min = t_inf;
    float pc_min = t_inf;
    tbb::mutex mutex;
    for (const auto& p : meshes_) {
        if (!p.second.enabled) {
            continue;
        }
        const TexturedMesh& mesh = p.second.mesh;
        if (p.second.pointCloud()) {
            std::vector<float> zs(mesh.vertices.size());
            for (std::size_t i = 0; i < zs.size(); ++i) {
                zs[i] = mesh.vertices[i][2];
            }
            int q10 = zs.size() / 10;
            std::nth_element(zs.begin(), zs.begin() + q10, zs.end());
            float t = (zs[q10] - ray.origin[2]) / ray.dir[2];
            if (t > 0 && t < pc_min) {
                tbb::mutex::scoped_lock lock(mutex);
                pc_min = t;
            }
        } else {
            SrsConv conv(camera_.srs(), mesh.srs);
            CameraRay localRay{ conv(ray.origin), ray.dir };
            tbb::parallel_for<std::size_t>(0, mesh.faces.size(), [&](std::size_t fi) {
                Triangle tri;
                for (int i = 0; i < 3; ++i) {
                    tri[i] = mesh.vertices[mesh.faces[fi][i]];
                }
                float t;
                if (intersection(localRay, tri, t) && t > 0 && t < mesh_min) {
                    tbb::mutex::scoped_lock lock(mutex);
                    mesh_min = t;
                }
            });
        }
    }
    float t_min = (mesh_min < t_inf) ? mesh_min : pc_min;
    if (t_min != t_inf) {
        Pvl::Vec3f target = ray.origin + ray.dir * t_min;
        camera_.lookAt(target);
    }
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
        TexturedMesh mesh = std::move(p.second->mesh);
        std::string basename = p.second->basename;
        deleteMesh(handle);
        Pvl::TriangleMesh<Pvl::Vec3f> trimesh;
        for (const Pvl::Vec3f& p : mesh.vertices) {
            trimesh.addVertex();
            trimesh.points.push_back(p);
        }
        /// \todo external point storage?
        mesh.vertices = {}; // free memory
        for (const TexturedMesh::Face& f : mesh.faces) {
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
            mesh.faces.push_back(TexturedMesh::Face{ face[0].index(), face[1].index(), face[2].index() });
        }
        view(handle, basename, std::move(mesh));
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

#ifdef HAS_OPENVDB

namespace {
class MeshAdapter {
    TexturedMesh& mesh_;
    openvdb::math::Transform::Ptr tr_;


public:
    MeshAdapter(TexturedMesh& mesh, openvdb::math::Transform::Ptr tr)
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

void repairMesh(TexturedMesh& mesh) {
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
        mesh.faces.emplace_back(TexturedMesh::Face{ f[0], f[2], f[1] });
    }
    for (auto& f : quads) {
        mesh.faces.emplace_back(TexturedMesh::Face{ f[0], f[2], f[1] });
        mesh.faces.emplace_back(TexturedMesh::Face{ f[0], f[3], f[2] });
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
        TexturedMesh mesh = std::move(p.second->mesh);
        std::string basename = p.second->basename;
        deleteMesh(handle);
        repairMesh(mesh);
        view(handle, basename, std::move(mesh));
    }

    camera_ = cameraState;
    update();
}

#else
void OpenGLWidget::repair() {}

#endif

void OpenGLWidget::estimateNormals(std::function<bool(float)> progress) {
    estimateNormals({}, progress);
}

struct TrajPoint {
    Coords p;
    double t;

    TrajPoint() = default;

    TrajPoint(const Coords& p, double t)
        : p(p)
        , t(t) {}

    bool operator<(const TrajPoint& other) const {
        return t < other.t;
    }
};

void OpenGLWidget::estimateNormals(const QString& trajectory, std::function<bool(float)> progress) {
    std::vector<std::pair<const void*, MeshData*>> meshData;
    // cannot erase from meshes_ while iterating, so add it to a vector
    for (auto& p : meshes_) {
        if (p.second.pointCloud()) {
            meshData.emplace_back(p.first, &p.second);
        }
    }
    // also backup camera
    auto cameraState = camera_;

    std::vector<TrajPoint> traj;
    if (!trajectory.isEmpty()) {
        std::ifstream ifs(trajectory.toStdString());
        std::string line;
        double t_min = 1.e20;
        double t_max = 0;
        while (std::getline(ifs, line)) {
            std::stringstream ss(line);
            double x, y, z, t;
            ss >> x >> y >> z >> t;
            traj.emplace_back(Coords(x, y, z), t);
            t_min = std::min(t_min, t);
            t_max = std::max(t_max, t);
        }
        std::sort(traj.begin(), traj.end());
        std::cout << "Trajectory time extent = " << t_min << " " << t_max << std::endl;
    }

    for (const auto& p : meshData) {
        /// \todo only update? (add normals)
        const void* handle = p.first;

        std::vector<Pvl::Vec3f>& points = p.second->mesh.vertices;
        std::vector<double>& times = p.second->mesh.times;
        std::vector<Pvl::Vec3f>& normals = p.second->mesh.normals;
        normals = Pvl::estimateNormals<Pvl::ParallelTag>(points, progress);

        if (!traj.empty() && !times.empty()) {
            std::size_t count = points.size();
            double t_min = 1.e20;
            double t_max = 0;
            for (std::size_t i = 0; i < count; ++i) {
                auto iter =
                    std::lower_bound(traj.begin(), traj.end(), times[i], [](const TrajPoint& tr, double t) {
                        return tr.t < t;
                    });
                Coords sensor;
                if (iter != traj.end()) {
                    sensor = iter->p;
                } else {
                    sensor = traj.back().p;
                }
                sensor = p.second->mesh.srs.worldToLocal(sensor);
                float dot = Pvl::dotProd(normals[i], vec3f(sensor) - points[i]);
                if (dot < 0) {
                    normals[i] *= -1;
                }
                t_min = std::min(t_min, times[i]);
                t_max = std::max(t_max, times[i]);
            }
            std::cout << "Lidar time extents = " << t_min << " " << t_max << std::endl;
        }

        TexturedMesh mesh = std::move(p.second->mesh);
        std::string basename = p.second->basename;
        deleteMesh(handle);
        view(handle, basename, std::move(mesh));
    }

    camera_ = cameraState;
    update();
}

void OpenGLWidget::computeAmbientOcclusion(std::function<bool(float)> progress) {
    std::vector<TexturedMesh> meshes;
    std::map<const void*, int> handleIndexMap;
    for (auto& p : meshes_) {
        const void* handle = p.first;
        if (p.second.pointCloud() || !p.second.vis.vertexColors.empty()) {
            // pc or already computed
            continue;
        }
        handleIndexMap[handle] = meshes.size();
        meshes.emplace_back(std::move(p.second.mesh));
    }
    if (!meshes.empty()) {
        if (!ambientOcclusion(meshes, progress)) {
            return;
        }
        for (auto& p : meshes_) {
            const void* handle = p.first;
            view(handle, p.second.basename, std::move(meshes[handleIndexMap[handle]]));
        }
    }
    enableAo(true);
}

bool OpenGLWidget::renderView() {
    std::vector<TexturedMesh*> meshesToRender;
    for (auto& p : meshes_) {
        if (p.second.pointCloud() || !p.second.enabled) {
            continue;
        }
        meshesToRender.push_back(&p.second.mesh);
    }
    if (meshesToRender.empty()) {
        return false;
    }
    FrameBufferWidget* frame = new FrameBufferWidget(this);
    frame->show();
    frame->run([this, frame, meshesToRender] {
        RenderWire wire = RenderWire::NOTHING;
        if (wireframe_) {
            wire = RenderWire::EDGES;
        } else if (dots_) {
            wire = RenderWire::DOTS;
        }
        renderMeshes(frame, meshesToRender, sunDir_, camera_, wire);
    });
    return true;
}
