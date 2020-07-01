#include "openglwidget.h"

#if 0
static void updateLights(const Camera& camera) {
    float dist = Pvl::norm(camera.eye() - camera.target()) * 100;
    Pvl::Mat33f mat = Pvl::invert(camera.matrix());
    Pvl::Vec3f pos1 = Pvl::prod(mat, Pvl::Vec3f(2, -2, -4)) * dist;
    Pvl::Vec3f pos2 = Pvl::prod(mat, Pvl::Vec3f(5, 0, -1)) * dist;
    glLightfv(GL_LIGHT0, GL_POSITION, reinterpret_cast<const float*>(&pos1));
    GLfloat diffuse1[] = { 1.1f, 1.05f, 1.f, 1.0 };
    // GLfloat diffuse1[] = { 0., 0., 0., 1.0 };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse1);

    glLightfv(GL_LIGHT1, GL_POSITION, reinterpret_cast<const float*>(&pos2));
    // GLfloat diffuse2[] = { 0.f, 0.f, 0.f, 1.f };
    GLfloat diffuse2[] = { 0.6f, 0.65f, 0.7f, 1.f };
    glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse2);
}
#else
static void updateLights(const Camera& camera) {
    float dist = Pvl::norm(camera.eye() - camera.target());
    // Pvl::Vec3f eye = camera.eye() * 1000.f;
    // GLfloat pos1[] = { eye[0], eye[1], eye[2] };
    GLfloat pos1[] = { 0, 0, 0, 1.e3f * dist };
    glLightfv(GL_LIGHT0, GL_POSITION, pos1);
    GLfloat diffuse1[] = { 1.f, 1.f, 1.f, 1.f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse1);
}
#endif

void OpenGLWidget::resizeGL(const int width, const int height) {
    std::cout << "Resizing " << width << " " << height << std::endl;
    glViewport(0, 0, width, height);

    if (!meshes_.empty()) {
        float dist = Pvl::norm(camera_.eye() - camera_.target());
        std::cout << "Resizing GL, dist = " << dist << std::endl;
        PVL_ASSERT(dist > 0.0001);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45, float(width) / height, 0.001 * dist, 1000. * dist);
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
    glEnable(GL_LIGHT0);
    GLfloat ambient[] = { 0, 0, 0, 1 };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
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

    glPointSize(2);
}

void OpenGLWidget::paintGL() {
    // std::cout << "Called paintGL" << std::endl;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glClearColor(0, 0.1, 0.3, 1);
    glClearColor(0.5, 0.5, 0.6, 1);
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

    gluPerspective(45, float(width()) / height(), 0.001 * dist, 1000. * dist);

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


    glColor3f(0.9, 0.9, 0.9);
    // glColor3f(0, 0, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    for (auto& p : meshes_) {
        if (!p.second.enabled) {
            continue;
        }

        if (vbos) {
            glBindBuffer(GL_ARRAY_BUFFER, p.second.vbo);
        }

        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_VERTEX_ARRAY);

        if (!vbos) {
            glVertexPointer(3, GL_FLOAT, 0, p.second.vis.vertices.data());
            glNormalPointer(GL_FLOAT, 0, p.second.vis.normals.data());
            glDrawArrays(GL_TRIANGLES, 0, p.second.vis.vertices.size() / 3);
        } else {
            glVertexPointer(3, GL_FLOAT, 0, (void*)0);
            glNormalPointer(GL_FLOAT, 0, (void*)(p.second.vis.vertices.size() * sizeof(float)));
            glDrawArrays(GL_TRIANGLES, 0, p.second.vis.vertices.size() / 3);
        }

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);

        if (vbos) {
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
            if (!p.second.enabled) {
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


    glFlush();
}

void OpenGLWidget::view(const void* handle, Mesh&& mesh) {
    bool firstMesh = meshes_.empty();
    MeshData& data = meshes_[handle];
    data.mesh = std::move(mesh);
    data.vis = {};

    for (std::size_t fi = 0; fi < data.mesh.faces.size(); ++fi) {
        Pvl::Vec3f normal = data.mesh.normal(fi);
        for (int i = 0; i < 3; ++i) {
            //    data.vis.indices.push_back(vh.index());
            const Pvl::Vec3f& vertex = data.mesh.vertices[data.mesh.faces[fi][i]];
            data.vis.vertices.push_back(vertex[0]);
            data.vis.vertices.push_back(vertex[1]);
            data.vis.vertices.push_back(vertex[2]);

            data.vis.normals.push_back(normal[0]);
            data.vis.normals.push_back(normal[1]);
            data.vis.normals.push_back(normal[2]);
        }
    }

    if (vbos) {
        glGenBuffers(1, &data.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, data.vbo);
        glBufferData(GL_ARRAY_BUFFER,
            (data.vis.vertices.size() + data.vis.normals.size()) * sizeof(float),
            0,
            GL_STATIC_DRAW);
        glBufferSubData(
            GL_ARRAY_BUFFER, 0, data.vis.vertices.size() * sizeof(float), data.vis.vertices.data());
        glBufferSubData(GL_ARRAY_BUFFER,
            data.vis.vertices.size() * sizeof(float),
            data.vis.normals.size() * sizeof(float),
            data.vis.normals.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    std::cout << "First mesh = " << firstMesh << std::endl;
    if (firstMesh) {

        Pvl::Box3f box;
        for (const Pvl::Vec3f& p : data.mesh.vertices) {
            box.extend(p);
        }
        Pvl::Vec3f center = box.center();
        float zoom = 1.5 * box.size()[0];

        camera_ = Camera(center + Pvl::Vec3f(0, 0, zoom),
            center,
            Pvl::Vec3f(0, 1, 0),
            M_PI / 4.,
            Pvl::Vec2i(width(), height()));

        /*    glViewport(0, 0, width(), height());

            float dist = Pvl::norm(camera_.eye() - camera_.target());
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            gluPerspective(45, float(width()) / height(), 0.01 * zoom, 100. * zoom);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            update();*/
        // resize(width(), height());
    }
    update();
}

void OpenGLWidget::updateCamera() {
    Pvl::Vec3f eye = camera_.eye();
    Pvl::Vec3f target = camera_.target();
    Pvl::Vec3f up = camera_.up();
    float aspect = 1.f; // float(width()) / height();
    camera_ = Camera(eye, target, up, M_PI / 4, Pvl::Vec2i(width(), height()));
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

    float t;
    float t_min = std::numeric_limits<float>::max();
    selected = Pvl::NONE;
    for (const auto& p : meshes_) {
        for (std::size_t fi = 0; fi < p.second.mesh.faces.size(); ++fi) {
            Triangle tri;
            for (int i = 0; i < 3; ++i) {
                tri[i] = p.second.mesh.vertices[p.second.mesh.faces[fi][i]];
            }
            if (intersection(ray, tri, t)) {
                std::cout << "Intersected triangle at " << t << std::endl;
                if (t > 0 && t < t_min) {
                    selected = tri;
                    t_min = t;
                }
            }
        }
    }

    if (selected) {
        Pvl::Vec3f target = ray.origin + ray.dir * t_min;
        camera_.lookAt(target);
    }
    selected = Pvl::NONE;
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
