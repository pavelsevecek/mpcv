#pragma once

#include "pvl/Box.hpp"
#include <cstdint>
#include <vector>

namespace Mpcv {

class Ray {
    friend bool intersectBox(const Pvl::Box3f& box, const Ray& ray, float& t_min, float& t_max);

private:
    Pvl::Vec3f orig;
    Pvl::Vec3f dir;
    Pvl::Vec3f invDir;
    std::array<int, 3> signs;

public:
    Ray() = default;

    Ray(const Pvl::Vec3f& origin, const Pvl::Vec3f& dir)
        : orig(origin)
        , dir(dir) {
        for (int i = 0; i < 3; ++i) {
            invDir[i] = (dir[i] == 0.f) ? INFINITY : 1.f / dir[i];
            signs[i] = int(invDir[i] < 0.f);
        }
    }

    const Pvl::Vec3f& origin() const {
        return orig;
    }

    const Pvl::Vec3f& direction() const {
        return dir;
    }
};

bool intersectBox(const Pvl::Box3f& box, const Ray& ray, float& t_min, float& t_max);

struct BvhPrimitive {
    /// Generic user data, can be used to store additional information to the primitives.
    int userData = int(-1);
};

/// \brief Holds intormation about intersection.
struct IntersectionInfo {
    /// Distance of the hit in units of ray.direction().
    float t;

    /// Object hit by the ray, or nullptr if nothing has been hit.
    const BvhPrimitive* object = nullptr;

    Pvl::Vec3f hit(const Ray& ray) const {
        return ray.origin() + ray.direction() * t;
    }

    /// Sort by intersection distance
    bool operator<(const IntersectionInfo& other) const {
        return t < other.t;
    }
};

/// \brief Trait for finding intersections with a triangle
class BvhTriangle : public BvhPrimitive {
private:
    Pvl::Vec3f v0;
    Pvl::Vec3f dir1, dir2;

public:
    BvhTriangle(const Pvl::Vec3f& v0, const Pvl::Vec3f& v1, const Pvl::Vec3f& v2, int data = -1)
        : v0(v0) {
        dir1 = v1 - v0;
        dir2 = v2 - v0;
        userData = data;
    }

    bool getIntersection(const Ray& ray, IntersectionInfo& intersection) const {
        // https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm#C++_Implementation

        const float eps = 1.e-12f;
        const Pvl::Vec3f h = crossProd(ray.direction(), dir2);
        const float a = dotProd(dir1, h);
        if (a > -eps && a < eps) {
            return false;
        }
        const float f = 1.f / a;
        const Pvl::Vec3f s = ray.origin() - v0;
        const float u = f * dotProd(s, h);
        if (u < -eps || u > 1.f + eps) {
            return false;
        }
        const Pvl::Vec3f q = crossProd(s, dir1);
        const float v = f * dotProd(ray.direction(), q);
        if (v < -eps || u + v > 1.f + eps) {
            return false;
        }
        const float t = f * dotProd(dir2, q);
        if (t <= 0) {
            return false;
        }
        intersection.object = this;
        intersection.t = t;
        return true;
    }

    Pvl::Box3f getBBox() const {
        Pvl::Box3f box;
        box.extend(v0);
        box.extend(v0 + dir1);
        box.extend(v0 + dir2);
        return box;
    }

    Pvl::Vec3f getCenter() const {
        return v0 + (dir1 + dir2) / 3.f;
    }

    std::array<Pvl::Vec3f, 3> getTriangle() const {
        return { v0, v0 + dir1, v0 + dir2 };
    }

    Pvl::Vec3f normal() const {
        Pvl::Vec3f n = crossProd(dir1, dir2);
        float length = norm(n);
        if (length > 1.e-20f) {
            return n / length;
        } else {
            return Pvl::Vec3f(0, 0, 1);
        }
    }
};

struct BvhNode {
    Pvl::Box3f box;
    uint32_t start;
    uint32_t primCnt;
    uint32_t rightOffset;
};

/// \brief Simple bounding volume hierarchy.
///
/// Interface for finding an intersection of given ray with a set of geometric objects. Currently very
/// limited and not very optimized. \ref Bvh is explicitly specialized for \ref BvhSphere and \ref BvhBox; if
/// other geometric primitives are needed, either add the specialization to cpp, or move the implementation to
/// header.
template <typename TBvhObject>
class Bvh {
private:
    const uint32_t leafSize;
    uint32_t nodeCnt = 0;
    uint32_t leafCnt = 0;

    std::vector<TBvhObject> objects;

    std::vector<BvhNode> nodes;

public:
    explicit Bvh(const uint32_t leafSize = 10)
        : leafSize(leafSize) {}

    /// \brief Contructs the BVH from given set of objects.
    ///
    /// This erased previously stored objects.
    void build(std::vector<TBvhObject>&& objects);

    /// \brief Releases the allocated data.
    void clear();

    bool getFirstIntersection(const Ray& ray, IntersectionInfo& intersection) const;

    /// \brief Returns true if the ray is occluded by some geometry
    bool isOccluded(const Ray& ray) const;

    /// \brief Returns the bounding box of all objects in BVH.
    Pvl::Box3f getBoundingBox() const;

private:
    template <typename TAddIntersection>
    void getIntersections(const Ray& ray, const TAddIntersection& addIntersection) const;
};

} // namespace Mpcv
