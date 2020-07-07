#pragma once

#include "pvl/Matrix.hpp"

namespace Mpcv {

class Quat {
    std::array<float, 4> v_;

public:
    Quat() = default;

    Quat(const Pvl::Vec3f& axis, const float angle) {
        Pvl::Vec3f normAxis = Pvl::normalize(axis);

        const float s = std::sin(0.5f * angle);
        const float c = std::cos(0.5f * angle);

        normAxis *= s;
        v_[0] = normAxis[0];
        v_[1] = normAxis[1];
        v_[2] = normAxis[2];
        v_[3] = c;
    }

    explicit Quat(const Pvl::Mat33f& m) {
        const float w = 0.5f * std::sqrt(1.f + m(0, 0) + m(1, 1) + m(2, 2));
        const float n = 0.25f / w;
        v_[0] = (m(2, 1) - m(1, 2)) * n;
        v_[1] = (m(0, 2) - m(2, 0)) * n;
        v_[2] = (m(1, 0) - m(0, 1)) * n;
        v_[3] = w;
    }

    Pvl::Vec3f axis() const {
        return Pvl::Vec3f(v_[0], v_[1], v_[2]) / std::sqrt(1.f - Pvl::sqr(v_[3]));
    }

    float angle() const {
        return acos(v_[3]) * 2.f;
    }

    float& operator[](const int idx) {
        return v_[idx];
    }

    float operator[](const int idx) const {
        return v_[idx];
    }

    Pvl::Mat33f convert() const {
        const float n = v_[0] * v_[0] + v_[1] * v_[1] + v_[2] * v_[2] + v_[3] * v_[3];
        const Pvl::Vec3f s = Pvl::Vec3f(v_[0], v_[1], v_[2]) * (n > 0.f ? 2.f / n : 0.f);
        const Pvl::Vec3f w = s * v_[3];

        const float xx = v_[0] * s[0];
        const float xy = v_[0] * s[1];
        const float xz = v_[0] * s[2];
        const float yy = v_[1] * s[1];
        const float yz = v_[1] * s[2];
        const float zz = v_[2] * s[2];

        return Pvl::Mat33f( //
            Pvl::Vec3f(1.f - yy - zz, xy - w[2], xz + w[1]),
            Pvl::Vec3f(xy + w[2], 1.f - xx - zz, yz - w[0]),
            Pvl::Vec3f(xz - w[1], yz + w[0], 1.f - xx - yy));
    }
};

class ArcBall {
    Pvl::Vec3f start_;
    Pvl::Vec2i size_ = Pvl::Vec2i(0, 0);
    Pvl::Mat33f mat_ = Pvl::Mat33f::identity();

public:
    ArcBall() = default;

    void initialize(const Pvl::Vec2i newSize, const Pvl::Mat33f& mat) {
        size_ = newSize;
        mat_ = mat;
    }

    void click(const Pvl::Vec2i point) {
        start_ = mapToSphere(point);
    }

    Pvl::Mat33f drag(const Pvl::Vec2i point) {
        const Pvl::Vec3f end = mapToSphere(point);
        const Pvl::Vec3f perp = Pvl::crossProd(start_, end);
        if (Pvl::normSqr(perp) > 1.e-6) {
            Quat q;
            q[0] = perp[0];
            q[1] = perp[1];
            q[2] = perp[2];
            q[3] = Pvl::dotProd(start_, end);

            return Pvl::invert(Pvl::invert(mat_) * q.convert() * mat_);
            //   return mat_ * q.convert() * Pvl::invert(mat_);
        } else {
            return Pvl::Mat33f::identity();
        }
    }

private:
    Pvl::Vec3f mapToSphere(const Pvl::Vec2i point) {
        // rescale to <-1, 1> and invert y
        const Pvl::Vec3f p(
            2.f * float(point[0]) / size_[0] - 1.f, 2.f * float(point[1]) / size_[1] - 1.f, 0.f);

        const float lengthSqr = Pvl::normSqr(p);
        if (lengthSqr > 1.f) {
            const float length = std::sqrt(lengthSqr);
            return p / length;
        } else {
            return Pvl::Vec3f(p[0], p[1], std::sqrt(1.f - lengthSqr));
        }
    }
};

} // namespace Mpcv
