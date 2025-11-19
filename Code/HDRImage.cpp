//
// Created by alex on 18/11/2025.
//

#include "HDRImage.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>


HDRImage::HDRImage(const std::string& filename) : m_width(0), m_height(0) {
    if (!load(filename)) {
        std::cerr << "Failed to load HDR image: " << filename << std::endl;
    }
}

bool HDRImage::load(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open PFM file " << filename << std::endl;
        return false;
    }

    std::string header;
    file >> header;

    if (header != "PF") {
        std::cerr << "Error: Unsupported PFM format (must be 'PF'): " << filename << std::endl;
        return false;
    }

    char nextChar;
    while (file >> std::ws) { // Skip whitespace
        nextChar = file.peek();
        if (nextChar == '#') {
            std::string comment;
            std::getline(file, comment); // Consume the entire comment line
        } else {
            break;
        }
    }

    file >> m_width >> m_height;

    float scale;
    file >> scale;

    char ch;
    while (file.get(ch)) {
        if (ch == '\n') break;
        if (!std::isspace(ch)) {
            std::cerr << "Error: Unexpected garbage in PFM header." << std::endl;
            return false;
        }
    }

    if (m_width <= 0 || m_height <= 0) {
        std::cerr << "Error: Invalid dimensions." << std::endl;
        return false;
    }

    m_data.resize(m_width * m_height * 3);

    // Read all float data
    file.read(reinterpret_cast<char*>(m_data.data()), m_data.size() * sizeof(float));

    if (!file) {
         std::cerr << "Error: File ended prematurely (read " << file.gcount() << " bytes)." << std::endl;
         return false;
    }

    bool file_is_big_endian = (scale > 0.0f);
    if (file_is_big_endian ) {
        std::cout << "Swapping endianness..." << std::endl;
        for (float& val : m_data) {
            char* bytes = reinterpret_cast<char*>(&val);
            std::swap(bytes[0], bytes[3]);
            std::swap(bytes[1], bytes[2]);
        }
    }

    // Apply Scale
    float absScale = std::abs(scale);
    if (absScale != 1.0f && absScale != 0.0f) {
        for (auto& f : m_data) f *= absScale;
    }

    return true;
}

Vector3 HDRImage::sample(double u, double v) const {
    // safety, if the image hasnt loaded correctly
    if (m_data.empty()) return Vector3(0, 0, 0);

    // convert spherical angles from radians to normalised texture coords
    // U becomes the horizontal percentage of the image width
    double U = u / (2.0 * M_PI);
    // V becomes the vertical percentage of the image height
    double V = v / M_PI;

    // Map UV to pixel coordinates
    // (0,0) is the bottom-left pixel.
    double px = U * (m_width  - 1);
    double py = V * (m_height - 1);

    // wrap the longitude. fmod can return a negative value
    px = fmod(px, m_width);
    // if fmod returns a negative, shift it from the other end of the image
    if (px < 0) px += m_width;

    // clamp the values to prevent an out of bounds pixel coord for safety
    py = std::clamp(py, 0.0, double(m_height - 1));

    // locate the 4 nearest integer coordinates by finding the bottom left pixel of a 2x2 block around the pixel
    int x0 = static_cast<int>(floor(px));
    int y0 = static_cast<int>(floor(py));

    // locate the top right pixel
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    // calculate a weight for how far the pixel is from (x0, y0)
    double dx = px - x0;
    double dy = py - y0;

    // for safety, add a wrap for the sampled pixel
    x1 = x1 % m_width;
    if (x1 < 0) x1 += m_width;

    // for safety, clamp the sampled pixel
    y1 = std::min(y1, m_height - 1);

    // helper to get a pixels RGB vector
    auto get_pixel = [&](int x, int y) -> Vector3 {
        size_t idx = (y * m_width + x) * 3;
        return Vector3(m_data[idx], m_data[idx+1], m_data[idx+2]);
    };

    Vector3 c00 = get_pixel(x0, y0); // Bottom-Left
    Vector3 c10 = get_pixel(x1, y0); // Bottom-Right
    Vector3 c01 = get_pixel(x0, y1); // Top-Left
    Vector3 c11 = get_pixel(x1, y1); // Top-Right

    // interpolate horizontally for each row according to the weights
    Vector3 bottom = c00 * (1.0 - dx) + c10 * dx;
    Vector3 top    = c01 * (1.0 - dx) + c11 * dx;

    // interpolate vertically for each row according to the weights
    return bottom * (1.0 - dy) + top * dy;

}