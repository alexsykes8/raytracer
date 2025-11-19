//
// Created by alex on 18/11/2025.
//

#ifndef B216602_HDRIMAGE_H
#define B216602_HDRIMAGE_H

#include <string>
#include <vector>
#include "vector3.h"

class HDRImage {
public:
    HDRImage() : m_width(0), m_height(0) {}
    explicit HDRImage(const std::string& filename);

    bool load(const std::string& filename);

    // Bilinear sampling of the HDR image using UV coordinates [0,1]
    Vector3 sample(double u, double v) const;

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    const std::vector<float>& getData() const { return m_data; }

private:
    int m_width;
    int m_height;
    std::vector<float> m_data; // Stored as R, G, B floats linearly
};

#endif //B216602_HDRIMAGE_H