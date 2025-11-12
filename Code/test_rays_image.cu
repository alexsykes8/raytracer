//
// Created by alex on 16/10/2025.
//

#include "test_rays_image.h"
#include <fstream>
#include <iostream>

Test_Image::Test_Image(int width, int height) : m_width(width), m_height(height) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Image dimensions must be positive.");
    }
    m_data.resize(width * height);
}

int Test_Image::getWidth() const {
    return m_width;
}

int Test_Image::getHeight() const {
    return m_height;
}

void Test_Image::setPixel(int x, int y, const Test_Pixel& p) {
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        m_data[y * m_width + x] = p;
    }
}

Test_Pixel Test_Image::getPixel(int x, int y) const {
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        return m_data[y * m_width + x];
    }
    throw std::out_of_range("Pixel coordinates are out of bounds.");
}

void Test_Image::write(const std::string& filename) const {
    std::ofstream outfile(filename, std::ios::binary);
    if (!outfile) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }

    // PPM header (P6 format for binary)
    outfile << "P6\n";
    outfile << m_width << " " << m_height << "\n";
    outfile << "255\n";

    // Write pixel data
    for (const auto& pixel : m_data) {
        outfile.put(pixel.r);
        outfile.put(pixel.g);
        outfile.put(pixel.b);
    }
}