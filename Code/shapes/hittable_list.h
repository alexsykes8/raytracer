//
// Created by alex on 29/10/2025.
//

#ifndef B216602_HITTABLE_LIST_H
#define B216602_HITTABLE_LIST_H

#include "hittable.h"
#include <vector>
#include <memory>
#include "../acceleration/aabb.h"

// Create list of objects in the scene
class HittableList : public Shape {
public:
    HittableList(){}

    void add(std::shared_ptr<Shape> object) {
        objects.push_back(object);
    }

    void clear() { objects.clear(); }

    // test the ray against every object in the list
    virtual bool intersect(const Ray &ray, double t_min, double t_max, HitRecord& rec) const override;

    virtual bool getBoundingBox(AABB &output_box) const override;


public:
    std::vector<std::shared_ptr<Shape>> objects;
};


#endif //B216602_HITTABLE_LIST_H