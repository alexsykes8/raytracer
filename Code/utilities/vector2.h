//
// Created by alex on 05/11/2025.
//

#ifndef B216602_VECTOR2_H
#define B216602_VECTOR2_H

// represents a 2d vector, commonly used for texture coordinates (u, v).
class Vector2 {
public:
    // the u and v components of the 2d vector.
    double u, v;

    // default constructor.
    // initialises the vector to (0.0, 0.0).
    Vector2() : u(0.0), v(0.0) {}

    // parameterised constructor.
    // initialises the vector with the given u and v values.
    Vector2(double u_in, double v_in) : u(u_in), v(v_in) {}
};

#endif //B216602_VECTOR2_H