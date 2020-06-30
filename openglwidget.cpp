#include "openglwidget.h"

void OpenGLWidget::resizeGL(const int width, const int height) {
    std::cout << "Resizing" << std::endl;
    glViewport(0, 0, width, height);

    bool first = width_ == 0;
    width_ = width;
    height_ = height;
    if (!first) {
        updateCamera();
        float dist = Pvl::norm(camera_.eye() - camera_.target());
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45, float(width) / height, 0.001 * dist, 1000. * dist);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

    } else {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45, float(width) / height, 1., 1000.);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }
}

void OpenGLWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0, 0, 0, 0);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glShadeModel(GL_FLAT);
    // glEnable(GL_AUTO_NORMAL);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
    GLfloat pos[] = { 0.5, 0, 2 };
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    GLfloat diffuse[] = { 0.6, 0.6, 0.6, 1.0 };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    GLfloat ambient[] = { 0.01, 0.01, 0.01, 1.0 };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

    glPointSize(4);
}

void OpenGLWidget::paintGL() {
    // std::cout << "Called paintGL" << std::endl;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0, 0.1, 0.3, 1);

    glLoadIdentity();
    {
        Pvl::Vec3f target = camera_.target();
        Pvl::Vec3f eye = camera_.eye();
        Pvl::Vec3f up = camera_.up();
        gluLookAt(eye[0], eye[1], eye[2], target[0], target[1], target[2], up[0], up[1], up[2]);
    }

    // glRotatef(angle_, 0, 0, 1);
    // glColor3f(1, 1, 1);
    /*glVertex3f(0.1, 0, 0);
    glVertex3f(0, 0, 0.1);
    glVertex3f(0, 0.1, 0);*/
    GLfloat color[] = { 0.5, 0.5, 0.5, 1.0 };
    GLfloat sel[] = { 1., 1., 0.0, 1.0 };
    glColor3fv(color);
    //  glMaterialfv(GL_FRONT, GL_DIFFUSE, color);
    glEnableClientState(GL_NORMAL_ARRAY);
    // glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);

    for (auto& p : meshes_) {
        if (!p.second.enabled) {
            continue;
        }
        glNormalPointer(GL_FLOAT, 0, p.second.vis.normals.data());
        glVertexPointer(3, GL_FLOAT, 0, p.second.vis.vertices.data());
        glDrawArrays(GL_TRIANGLES, 0, p.second.vis.vertices.size() / 3);

        /*for (auto& t : p.second.mesh) {
            if (&t == selected) {
                glColor3fv(sel);
            } else {
                glColor3fv(color);
            }
            Pvl::Vec3f n = Pvl::normalize(Pvl::crossProd(t[1] - t[0], t[2] - t[0]));
            glNormal3f(n[0], n[1], n[2]);
            glVertex3f(t[0][0], t[0][1], t[0][2]);
            glNormal3f(n[0], n[1], n[2]);
            glVertex3f(t[1][0], t[1][1], t[1][2]);
            glNormal3f(n[0], n[1], n[2]);
            glVertex3f(t[2][0], t[2][1], t[2][2]);

        }*/
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glFlush();
}

void OpenGLWidget::view(const void* handle, Pvl::TriangleMesh<Pvl::Vec3f>&& mesh) {
    MeshData& data = meshes_[handle];
    data.mesh = std::move(mesh);
    data.vis = {};
    for (Pvl::FaceHandle fh : data.mesh.faceRange()) {
        Pvl::Vec3f normal = data.mesh.normal(fh);
        for (Pvl::VertexHandle vh : data.mesh.vertexRing(fh)) {
            //    data.vis.indices.push_back(vh.index());
            Pvl::Vec3f vertex = data.mesh.point(vh);
            data.vis.vertices.push_back(vertex[0]);
            data.vis.vertices.push_back(vertex[1]);
            data.vis.vertices.push_back(vertex[2]);

            data.vis.normals.push_back(normal[0]);
            data.vis.normals.push_back(normal[1]);
            data.vis.normals.push_back(normal[2]);
        }
    }
    Pvl::Box3f box;
    for (Pvl::VertexHandle vh : data.mesh.vertexRange()) {
        box.extend(data.mesh.point(vh));
    }
    Pvl::Vec3f center = box.center();
    float zoom = 2 * box.size()[0];

    camera_ = Camera(
        center + Pvl::Vec3f(0, 0, zoom), center, Pvl::Vec3f(0, 1, 0), M_PI / 4., Pvl::Vec2i(width_, height_));
    //   mesh_ = mesh;
}


void OpenGLWidget::mousePressEvent(QMouseEvent* event) {
    mouse_.pos0 = event->pos();
    mouse_.state = camera_;
    if (event->button() == Qt::RightButton) {
        mouse_.ab.initialize(Pvl::Vec2i(width_, height_), camera_.matrix());
        mouse_.ab.click(Pvl::Vec2i(mouse_.pos0.x(), mouse_.pos0.y()));
    }
}

void OpenGLWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    mouse_.pos0 = event->pos();

    std::cout << "Clicked at " << mouse_.pos0.x() << "," << mouse_.pos0.y() << std::endl;
    Ray ray = camera_.project(Pvl::Vec2f(mouse_.pos0.x(), mouse_.pos0.y()));

    float t;
    float t_min = std::numeric_limits<float>::max();
    const Triangle* tri_min = nullptr;
    for (const auto& p : meshes_) {
        for (Pvl::FaceHandle fh : p.second.mesh.faceRange()) {
            Triangle tri = p.second.mesh.triangle(fh);
            if (intersection(ray, tri, t)) {
                std::cout << "Intersected triangle at " << t << std::endl;
                if (t < t_min) {
                    tri_min = &tri;
                    t_min = t;
                }
            }
        }
    }
    // selected = tri_min;
    if (tri_min) {
        Pvl::Vec3f target = ray.origin + ray.dir * t_min;
        camera_.lookAt(target);
    }
    update();
}

void OpenGLWidget::mouseMoveEvent(QMouseEvent* ev) {
    camera_ = mouse_.state;
    if (ev->buttons() & Qt::RightButton) {
        QPoint p = ev->pos();
        Pvl::Mat33f m = mouse_.ab.drag(Pvl::Vec2i(p.x(), p.y()));
        camera_.transform(m);
    } else {
        QPoint dp = ev->pos() - mouse_.pos0;
        camera_.pan(Pvl::Vec2i(dp.x(), dp.y()));
    }

    // mouse_.pos0 = ev->pos();
    update();
}
