#include "camera.h"

namespace Mpcv {

Camera::Camera(Pvl::Vec3f eye, Pvl::Vec3f target, Pvl::Vec3f up, float fov_y, const Srs& srs, Pvl::Vec2i size)
    : eye_(eye)
    , target_(target)
    , up_(up)
    , size_(size)
    , srs_(srs) {
    dir_ = Pvl::normalize(target - eye);
    up_ = up;
    up_ = Pvl::normalize(up_ - Pvl::dotProd(up_, dir_) * dir_);

    const float aspect = float(size_[0]) / float(size_[1]);
    const float tgfov = tan(0.5f * fov_y);
    up_ = tgfov * up_;
    left_ = aspect * tgfov * Pvl::normalize(Pvl::crossProd(up_, dir_));
    std::cout << "Creating camera with aspect " << aspect << std::endl;
}

void Camera::zoom(float factor) {
    float dist = Pvl::norm(eye_ - target_);
    eye_ = target_ - dir_ * dist * factor;
}

void Camera::pan(const Pvl::Vec2i& dp) {
    const float x = float(dp[0]) / size_[0];
    const float y = float(dp[1]) / size_[1];
    const Pvl::Vec3f offset = 2.f * Pvl::norm(target_ - eye_) * (left_ * x + up_ * y);
    eye_ += offset;
    target_ += offset;
}

void Camera::transform(const Pvl::Mat33f& m) {
    dir_ = Pvl::prod(m, dir_);
    up_ = Pvl::prod(m, up_);
    left_ = Pvl::prod(m, left_);

    float dist = Pvl::norm(eye_ - target_);
    eye_ = target_ - dir_ * dist;
}

void Camera::lookAt(const Pvl::Vec3f& pos) {
    Pvl::Vec3f offset = pos - target_;
    target_ += offset;
    eye_ += offset;
}

CameraRay Camera::project(const Pvl::Vec2f& coords) const {
    const float rx = 2.f * coords[0] / size_[0] - 1.f;
    const float ry = 2.f * coords[1] / size_[1] - 1.f;
    const Pvl::Vec3f dir = dir_ - left_ * rx - up_ * ry;
    CameraRay ray;
    ray.origin = eye_;
    ray.dir = Pvl::normalize(dir);
    return ray;
}

Pvl::Optional<Pvl::Vec2f> Camera::unproject(const Pvl::Vec3f& r) const {
    const Pvl::Vec3f dr = r - eye_;
    const float proj = Pvl::dotProd(dr, dir_);
    if (proj <= 0.f) {
        return Pvl::NONE;
    }
    const Pvl::Vec3f r0 = dr / proj;
    // convert [-1, 1] to [0, imageSize]
    Pvl::Vec3f left0 = Pvl::normalize(left_);
    float leftLength = Pvl::norm(left_);
    Pvl::Vec3f up0 = Pvl::normalize(up_);
    float upLength = Pvl::norm(up_);
    const float leftRel = float(Pvl::dotProd(left0, r0) / leftLength);
    const float upRel = float(Pvl::dotProd(up0, r0) / upLength);
    const float x = 0.5f * (1.f - leftRel) * size_[0];
    const float y = 0.5f * (1.f - upRel) * size_[1];
    return Pvl::Vec2f(x, y);
}


bool intersection(const CameraRay& ray, const Triangle& tri, float& t) {
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

bool intersection(const CameraRay& ray, const Pvl::Vec3f& point, float radius, float& t) {
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


} // namespace Mpcv
