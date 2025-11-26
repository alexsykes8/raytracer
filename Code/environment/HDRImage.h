//
// Created by alex on 18/11/2025.
//

#ifndef B216602_HDRIMAGE_H
#define B216602_HDRIMAGE_H

#include <string>
#include <vector>
#include "../utilities/vector3.h"

class HDRImage {
public:
    // default constructor.
    // initialises an empty image with zero width and height.
    HDRImage() : m_width(0), m_height(0) {}
    // 'explicit' prevents the compiler from performing implicit type conversions from a string to an hdrimage.
    // constructor that loads an hdr image from a file path.
    explicit HDRImage(const std::string& filename);

    // loads an hdr image from a .pfm file.
    bool load(const std::string& filename);

    // performs bilinear sampling of the hdr image using spherical coordinates (u, v).
    Vector3 sample(double u, double v) const;

    // returns the width of the image in pixels.
    int getWidth() const { return m_width; }
    // returns the height of the image in pixels.
    int getHeight() const { return m_height; }
    // returns a constant reference to the raw pixel data.
    const std::vector<float>& getData() const { return m_data; }

private:
    // the width of the image in pixels.
    int m_width;
    // the height of the image in pixels.
    int m_height;
    // 'std::vector' is a sequence container that encapsulates a dynamic size array.
    // the pixel data, stored as a flat 1d vector of floating-point rgb values.
    std::vector<float> m_data;
};

#endif //B216602_HDRIMAGE_H