#pragma once

#include "pvl/Matrix.hpp"
#include "pvl/Optional.hpp"
#include <iostream>

struct Ray {
    Pvl::Vec3f origin;
    Pvl::Vec3f dir;
};

class Camera {
    Pvl::Vec3f eye_;
    Pvl::Vec3f target_;
    Pvl::Vec3f dir_;
    Pvl::Vec3f up_;
    Pvl::Vec3f left_;
    Pvl::Vec2i size_;

public:
    Camera() = default;

    Camera(Pvl::Vec3f eye, Pvl::Vec3f target, Pvl::Vec3f up, float fov_y, Pvl::Vec2i size)
        : eye_(eye)
        , target_(target)
        , up_(up)
        , size_(size) {
        dir_ = Pvl::normalize(target - eye);
        up_ = up;
        up_ = Pvl::normalize(up_ - Pvl::dotProd(up_, dir_) * dir_);

        const float aspect = float(size_[0]) / float(size_[1]);
        const float tgfov = tan(0.5f * fov_y);
        up_ = tgfov * up_;
        left_ = aspect * tgfov * Pvl::normalize(Pvl::crossProd(up_, dir_));
        std::cout << "Creating camera with aspect " << aspect << std::endl;
    }

    Pvl::Vec3f eye() const {
        return eye_;
    }

    Pvl::Vec3f target() const {
        return target_;
    }

    Pvl::Vec3f up() const {
        return up_;
    }

    Pvl::Vec3f direction() const {
        return dir_;
    }

    Pvl::Mat33f matrix() const {
        return Pvl::Mat33f(Pvl::normalize(left_), Pvl::normalize(up_), dir_);
    }

    void zoom(float factor) {
        float dist = Pvl::norm(eye_ - target_);
        eye_ = target_ - dir_ * dist * factor;
    }

    void pan(const Pvl::Vec2i& dp) {
        const float x = float(dp[0]) / size_[0];
        const float y = float(dp[1]) / size_[1];
        const Pvl::Vec3f offset = 2.f * Pvl::norm(target_ - eye_) * (left_ * x + up_ * y);
        eye_ += offset;
        target_ += offset;
    }

    void transform(const Pvl::Mat33f& m) {
        dir_ = Pvl::prod(m, dir_);
        up_ = Pvl::prod(m, up_);
        left_ = Pvl::prod(m, left_);

        float dist = Pvl::norm(eye_ - target_);
        eye_ = target_ - dir_ * dist;
    }

    void lookAt(const Pvl::Vec3f& pos) {
        Pvl::Vec3f offset = pos - target_;
        target_ += offset;
        eye_ += offset;
    }

    Ray project(const Pvl::Vec2f& coords) const {
        const float rx = 2.f * coords[0] / size_[0] - 1.f;
        const float ry = 2.f * coords[1] / size_[1] - 1.f;
        const Pvl::Vec3f dir = dir_ - left_ * rx - up_ * ry;
        Ray ray;
        ray.origin = eye_;
        ray.dir = dir;
        return ray;
    }

    Pvl::Optional<Pvl::Vec2f> unproject(const Pvl::Vec3f& r) const {
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

    Pvl::Vec2i dimensions() const {
        return size_;
    }
};
