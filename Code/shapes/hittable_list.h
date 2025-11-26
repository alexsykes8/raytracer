//
// Created by alex on 29/10/2025.
//

#ifndef B216602_HITTABLE_LIST_H
#define B216602_HITTABLE_LIST_H

#include "hittable.h"
#include <vector>
#include <memory>
#include "../acceleration/aabb.h"

// a container class that holds a list of 'shape' objects and acts as a single 'shape' itself.
class HittableList : public Shape {
public:
    // default constructor.
    HittableList(){}

    // adds a shape to the list.
    // 'std::shared_ptr' is a smart pointer that retains shared ownership of an object through a pointer.
    void add(std::shared_ptr<Shape> object) {
        objects.push_back(object);
    }

    // removes all shapes from the list.
    void clear() { objects.clear(); }


    // overrides the base class method to test the ray against every object in the list.
    virtual bool intersect(const Ray &ray, double t_min, double t_max, HitRecord& rec) const override;

    // overrides the base class method to compute a bounding box that encloses all objects in the list.
    virtual bool getBoundingBox(AABB &output_box) const override;


public:
    // a list of smart pointers to the shape objects in the scene.
    // 'std::vector' is a sequence container that encapsulates dynamic size arrays.
    std::vector<std::shared_ptr<Shape>> objects;
};


#endif //B216602_HITTABLE_LIST_H