#include "openglwidget.h"

void OpenGLWidget::resizeGL(const int width, const int height) {
    std::cout << "Resizing" << std::endl;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, float(width) / height, 0.001, 1.);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    bool first = width_ == 0;
    width_ = width;
    height_ = height;
    if (!first) {
        updateCamera();
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
    glBegin(GL_TRIANGLES);
    // glColor3f(1, 1, 1);
    /*glVertex3f(0.1, 0, 0);
    glVertex3f(0, 0, 0.1);
    glVertex3f(0, 0.1, 0);*/
    GLfloat color[] = { 0.5, 0.5, 0.5, 1.0 };
    GLfloat sel[] = { 1., 1., 0.0, 1.0 };
    glColor3fv(color);
    //  glMaterialfv(GL_FRONT, GL_DIFFUSE, color);
    for (auto& p : meshes_) {
        if (!p.second.enabled) {
            continue;
        }
        for (auto& t : p.second.mesh) {
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
        }
    }
    glEnd();
    glFlush();
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
        for (const auto& tri : p.second.mesh) {
            if (intersection(ray, tri, t)) {
                std::cout << "Intersected triangle at " << t << std::endl;
                if (t < t_min) {
                    tri_min = &tri;
                    t_min = t;
                }
            }
        }
    }
    selected = tri_min;
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
