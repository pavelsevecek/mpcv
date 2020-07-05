#include "bvh.h"

namespace Pvl {

struct BvhTraversal {
    uint32_t idx;
    float t_min;
};

struct BvhBuildEntry {
    uint32_t parent;
    uint32_t start;
    uint32_t end;
};

bool intersectBox(const Box3f& box, const Ray& ray, float& t_min, float& t_max) {
    std::array<Vec3f, 2> b = { box.lower(), box.upper() };
    float tmin = (b[ray.signs[0]][0] - ray.orig[0]) * ray.invDir[0];
    float tmax = (b[1 - ray.signs[0]][0] - ray.orig[0]) * ray.invDir[0];
    PVL_ASSERT(!std::isnan(tmin) && !std::isnan(tmax)); // they may be inf though
    const float tymin = (b[ray.signs[1]][1] - ray.orig[1]) * ray.invDir[1];
    const float tymax = (b[1 - ray.signs[1]][1] - ray.orig[1]) * ray.invDir[1];
    PVL_ASSERT(!std::isnan(tymin) && !std::isnan(tymax));

    if ((tmin > tymax) || (tymin > tmax)) {
        return false;
    }
    tmin = std::max(tmin, tymin);
    tmax = std::min(tmax, tymax);

    const float tzmin = (b[ray.signs[2]][2] - ray.orig[2]) * ray.invDir[2];
    const float tzmax = (b[1 - ray.signs[2]][2] - ray.orig[2]) * ray.invDir[2];
    PVL_ASSERT(!std::isnan(tzmin) && !std::isnan(tzmax));

    if ((tmin > tzmax) || (tzmin > tmax)) {
        return false;
    }
    tmin = std::max(tmin, tzmin);
    tmax = std::min(tmax, tzmax);

    t_min = tmin;
    t_max = tmax;

    return true;
}

template <typename TBvhObject>
template <typename TAddIntersection>
void Bvh<TBvhObject>::getIntersections(const Ray& ray, const TAddIntersection& addIntersection) const {
    std::array<float, 4> boxHits;
    uint32_t closer;
    uint32_t other;

    std::array<BvhTraversal, 64> stack;
    int stackIdx = 0;

    stack[stackIdx].idx = 0;
    stack[stackIdx].t_min = 0.f; // -INFTY;

    while (stackIdx >= 0) {
        const uint32_t idx = stack[stackIdx].idx;
        // const float t_min = stack[stackIdx].t_min;
        stackIdx--;
        const BvhNode& node = nodes[idx];

        /// \todo optimization for the single intersection case
        /*        if (t_min > intersection.t) {
                    continue;
                }*/
        if (node.rightOffset == 0) {
            // leaf
            for (uint32_t primIdx = 0; primIdx < node.primCnt; ++primIdx) {
                IntersectionInfo current;

                const TBvhObject& obj = objects[node.start + primIdx];
                const bool hit = obj.getIntersection(ray, current);

                if (hit && !addIntersection(current)) {
                    // bailout
                    return;
                }
            }
        } else {
            // inner node
            const bool hitc0 = intersectBox(nodes[idx + 1].box, ray, boxHits[0], boxHits[1]);
            const bool hitc1 = intersectBox(nodes[idx + node.rightOffset].box, ray, boxHits[2], boxHits[3]);

            if (hitc0 && hitc1) {
                closer = idx + 1;
                other = idx + node.rightOffset;

                if (boxHits[2] < boxHits[0]) {
                    std::swap(boxHits[0], boxHits[2]);
                    std::swap(boxHits[1], boxHits[3]);
                    std::swap(closer, other);
                }

                if (boxHits[3] > 0) {
                    stack[++stackIdx] = BvhTraversal{ other, boxHits[2] };
                }
                if (boxHits[1] > 0) {
                    stack[++stackIdx] = BvhTraversal{ closer, boxHits[0] };
                }
            } else if (hitc0 && boxHits[1] > 0) {
                stack[++stackIdx] = BvhTraversal{ idx + 1, boxHits[0] };
            } else if (hitc1 && boxHits[3] > 0) {
                stack[++stackIdx] = BvhTraversal{ idx + node.rightOffset, boxHits[2] };
            }
        }
    }
}

template <typename TBvhObject>
bool Bvh<TBvhObject>::getFirstIntersection(const Ray& ray, IntersectionInfo& intersection) const {
    intersection.t = std::numeric_limits<float>::max();
    intersection.object = nullptr;

    this->getIntersections(ray, [&intersection](IntersectionInfo& current) {
        if (current.t > 0 && current.t < intersection.t) {
            intersection = current;
        }
        return true;
    });
    return intersection.object != nullptr;
}

template <typename TBvhObject>
bool Bvh<TBvhObject>::isOccluded(const Ray& ray) const {
    bool occluded = false;
    getIntersections(ray, [&occluded](IntersectionInfo&) {
        occluded = true;
        return false; // do not continue with traversal
    });
    return occluded;
}

template <typename TBvhObject>
void Bvh<TBvhObject>::build(std::vector<TBvhObject>&& objs) {
    objects = std::move(objs);
    PVL_ASSERT(!objects.empty());
    nodeCnt = 0;
    leafCnt = 0;

    std::array<BvhBuildEntry, 128> stack;
    uint32_t stackIdx = 0;
    constexpr uint32_t UNTOUCHED = 0xffffffff;
    constexpr uint32_t NO_PARENT = 0xfffffffc;
    constexpr uint32_t TOUCHED_TWICE = 0xfffffffd;

    // Push the root
    stack[stackIdx].start = 0;
    stack[stackIdx].end = objects.size();
    stack[stackIdx].parent = NO_PARENT;
    stackIdx++;

    BvhNode node;
    std::vector<BvhNode> buildNodes;
    buildNodes.reserve(2 * objects.size());

    while (stackIdx > 0) {
        BvhBuildEntry& nodeEntry = stack[--stackIdx];
        const uint32_t start = nodeEntry.start;
        const uint32_t end = nodeEntry.end;
        const uint32_t primCnt = end - start;

        nodeCnt++;
        node.start = start;
        node.primCnt = primCnt;
        node.rightOffset = UNTOUCHED;

        Box3f bbox = objects[start].getBBox();
        const Vec3f center = objects[start].getCenter();
        Box3f boxCenter(center, center);
        for (uint32_t i = start + 1; i < end; ++i) {
            bbox.extend(objects[i].getBBox());
            boxCenter.extend(objects[i].getCenter());
        }
        node.box = bbox;

        if (primCnt <= leafSize) {
            node.rightOffset = 0;
            leafCnt++;
        }
        buildNodes.push_back(node);

        if (nodeEntry.parent != NO_PARENT) {
            buildNodes[nodeEntry.parent].rightOffset--;

            if (buildNodes[nodeEntry.parent].rightOffset == TOUCHED_TWICE) {
                buildNodes[nodeEntry.parent].rightOffset = nodeCnt - 1 - nodeEntry.parent;
            }
        }

        if (node.rightOffset == 0) {
            continue;
        }

        const uint32_t splitDim = argMax(boxCenter.size());
        const float split = 0.5f * (boxCenter.lower()[splitDim] + boxCenter.upper()[splitDim]);

        uint32_t mid = start;
        for (uint32_t i = start; i < end; ++i) {
            if (objects[i].getCenter()[splitDim] < split) {
                std::swap(objects[i], objects[mid]);
                ++mid;
            }
        }

        if (mid == start || mid == end) {
            mid = start + (end - start) / 2;
        }

        stack[stackIdx++] = { nodeCnt - 1, mid, end };
        stack[stackIdx++] = { nodeCnt - 1, start, mid };
    }

    PVL_ASSERT(buildNodes.size() == nodeCnt);
    nodes = std::move(buildNodes);
}

template <typename TBvhObject>
Box3f Bvh<TBvhObject>::getBoundingBox() const {
    return nodes[0].box;
}

template class Bvh<BvhTriangle>;


} // namespace Pvl
