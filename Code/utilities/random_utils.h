//
// Created by alex on 20/11/2025.
//

#include <random>


#ifndef B216602_RANDOM_UTILS_H
#define B216602_RANDOM_UTILS_H

// this is in a new header to improve performance when generating random numbers across parallel threads

inline double random_double() {
    static thread_local std::mt19937 generator(std::random_device{}());
    static thread_local std::uniform_real_distribution<double> distribution(0.0, 1.0);
    return distribution(generator);
}

inline double random_double(double min, double max) {
    return min + (max - min) * random_double();
}

inline int random_int(int min, int max) {
    static thread_local std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(generator);
}

inline Vector3 random_in_unit_sphere() {
    while (true) {
        auto p = Vector3(random_double() * 2 - 1, random_double() * 2 - 1, random_double() * 2 - 1);
        if (p.dot(p) < 1.0)
            return p;
    }
}

#endif //B216602_RANDOM_UTILS_H