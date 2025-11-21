#include "bvh.h"
#include <iostream>
#include <random>


// Comparator function for sorting shapes based on their bounding box center along an axis
inline bool boxCompare(const std::shared_ptr<Shape>& a, const std::shared_ptr<Shape>& b, int axis) {
    AABB box_a, box_b;
    if (!a->getBoundingBox(box_a) || !b->getBoundingBox(box_b)) {
        std::cerr << "Error: No bounding box in BVHNode constructor.\n";
    }
    
    // Calculate centers
    Vector3 center_a = (box_a.min_point + box_a.max_point) * 0.5;
    Vector3 center_b = (box_b.min_point + box_b.max_point) * 0.5;

    if (axis == 0) return center_a.x < center_b.x;
    if (axis == 1) return center_a.y < center_b.y;
    return center_a.z < center_b.z;
}

bool boxCompareX(const std::shared_ptr<Shape>& a, const std::shared_ptr<Shape>& b) {
    return boxCompare(a, b, 0);
}
bool boxCompareY(const std::shared_ptr<Shape>& a, const std::shared_ptr<Shape>& b) {
    return boxCompare(a, b, 1);
}
bool boxCompareZ(const std::shared_ptr<Shape>& a, const std::shared_ptr<Shape>& b) {
    return boxCompare(a, b, 2);
}

BVHNode::BVHNode(std::vector<std::shared_ptr<Shape>>& objects, size_t start, size_t end) {
    AABB span_box;
    bool first_box = true;

    // Compute the bounding box spanning all objects in this list segment
    for (size_t i = start; i < end; i++) {
        AABB temp_box;
        if (objects[i]->getBoundingBox(temp_box)) {
            if (first_box) {
                span_box = temp_box;
                first_box = false;
            } else {
                span_box = AABB::combine(span_box, temp_box);
            }
        }
    }
    Vector3 extent = span_box.max_point - span_box.min_point;
    int axis = 0;

    // Pick the axis with the largest extent
    if (extent.y > extent.x && extent.y > extent.z) {
        axis = 1; // Y is longest
    } else if (extent.z > extent.x && extent.z > extent.y) {
        axis = 2; // Z is longest
    } else {
        axis = 0; // X is longest (or default)
    }
    
    auto comparator = (axis == 0) ? boxCompareX : (axis == 1) ? boxCompareY : boxCompareZ;

    size_t object_span = end - start;

    if (object_span == 1) {
        // Base case: Leaf node with one object
        m_left = m_right = objects[start];
    } else if (object_span == 2) {
        // Base case: Leaf node with two objects
        if (comparator(objects[start], objects[start+1])) {
            m_left = objects[start];
            m_right = objects[start+1];
        } else {
            m_left = objects[start+1];
            m_right = objects[start];
        }
    } else {
        // General case: Sort and split
        auto mid_iter = objects.begin() + start + object_span / 2;
        std::nth_element(objects.begin() + start, mid_iter, objects.begin() + end, comparator);

        size_t mid = start + object_span / 2;
        m_left = std::make_shared<BVHNode>(objects, start, mid);
        m_right = std::make_shared<BVHNode>(objects, mid, end);
    }

    // Calculate this node's bounding box
    AABB box_left, box_right;
    if (!m_left->getBoundingBox(box_left) || !m_right->getBoundingBox(box_right)) {
        std::cerr << "Error: No bounding box in BVHNode constructor children.\n";
    }
    m_box = AABB::combine(box_left, box_right);
}


bool BVHNode::getBoundingBox(AABB& output_box) const {
    output_box = m_box;
    return true;
}


bool BVHNode::intersect(const Ray& ray, double t_min, double t_max, HitRecord& rec) const {
    // Check if the ray hits this node's bounding box
    if (!m_box.intersect(ray, t_min, t_max)) {
        return false; // Missed the box, prune this branch
    }

    // Ray hit the box, recursively check children
    bool hit_left = m_left->intersect(ray, t_min, t_max, rec);
    
    // Update t_max for the right child test. If the left child is hit, only closer hits matter.
    bool hit_right = m_right->intersect(ray, t_min, (hit_left ? rec.t : t_max), rec);

    return hit_left || hit_right;
}

