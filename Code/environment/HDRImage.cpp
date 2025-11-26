//
// Created by alex on 18/11/2025.
//

#include "HDRImage.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>


// constructor that loads an hdr image from a file path.
HDRImage::HDRImage(const std::string& filename) : m_width(0), m_height(0) {
    // attempts to load the image and prints an error if it fails.
    if (!load(filename)) {
        std::cerr << "Failed to load HDR image: " << filename << std::endl;
    }
}

// loads image data from a portable floatmap (.pfm) file.
bool HDRImage::load(const std::string& filename) {
    // 'std::ifstream' is a class for input file stream operations.
    // 'std::ios::binary' is a flag to open the file in binary mode.
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open PFM file " << filename << std::endl;
        return false;
    }

    // reads the file identifier from the header.
    std::string header;
    file >> header;

    // validates the file format. only 'pf' (colour) is supported.
    if (header != "PF") {
        std::cerr << "Error: Unsupported PFM format (must be 'PF'): " << filename << std::endl;
        return false;
    }

    // skips any whitespace and comment lines in the header.
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

    // reads the image dimensions from the header.
    file >> m_width >> m_height;

    // reads the scale factor/endianness identifier from the header.
    float scale;
    file >> scale;

    // consumes the single newline character that separates the header from the data.
    char ch;
    while (file.get(ch)) {
        if (ch == '\n') break;
        if (!std::isspace(ch)) {
            std::cerr << "Error: Unexpected garbage in PFM header." << std::endl;
            return false;
        }
    }

    // validates that the image dimensions are positive.
    if (m_width <= 0 || m_height <= 0) {
        std::cerr << "Error: Invalid dimensions." << std::endl;
        return false;
    }

    // resizes the data vector to hold all pixel data (width * height * 3 floats for rgb).
    m_data.resize(m_width * m_height * 3);

    // reads the entire block of binary pixel data at once.
    // 'reinterpret_cast' converts a pointer to any other pointer type, used here to treat the float vector as a char array for reading.
    file.read(reinterpret_cast<char*>(m_data.data()), m_data.size() * sizeof(float));

    // checks for read errors.
    if (!file) {
         std::cerr << "Error: File ended prematurely (read " << file.gcount() << " bytes)." << std::endl;
         return false;
    }

    // determines the endianness of the file from the sign of the scale factor.
    bool file_is_big_endian = (scale > 0.0f);
    if (file_is_big_endian ) {
        std::cout << "Swapping endianness..." << std::endl;
        // if the file is big-endian, swap the byte order for each float to match the (typically) little-endian system.
        for (float& val : m_data) {
            char* bytes = reinterpret_cast<char*>(&val);
            std::swap(bytes[0], bytes[3]);
            std::swap(bytes[1], bytes[2]);
        }
    }

    // applies the scale factor to each colour component.
    float absScale = std::abs(scale);
    if (absScale != 1.0f && absScale != 0.0f) {
        // 'auto' means that the compiler deduces the type of a variable from its initialiser.
        for (auto& f : m_data) f *= absScale;
    }

    return true;
}

// performs bilinear interpolation to get a smooth pixel colour from spherical coordinates.
Vector3 HDRImage::sample(double u, double v) const {
    // returns black if the image data is not loaded.
    if (m_data.empty()) return Vector3(0, 0, 0);

    // converts the spherical angle u (longitude, in radians) to a normalised horizontal texture coordinate.
    // equation: u_norm = u / (2 * π)
    double U = u / (2.0 * M_PI);
    // converts the spherical angle v (latitude, in radians) to a normalised vertical texture coordinate.
    // equation: v_norm = v / π
    double V = v / M_PI;

    // maps normalised coordinates to floating-point pixel coordinates.
    // equation: px = u_norm * (width - 1)
    double px = U * (m_width  - 1);
    // equation: py = v_norm * (height - 1)
    double py = V * (m_height - 1);

    // wraps the horizontal coordinate for seamless texturing (longitude wraps around).
    px = fmod(px, m_width);
    if (px < 0) px += m_width;

    // clamps the vertical coordinate to the image boundaries (latitude does not wrap).
    py = std::clamp(py, 0.0, double(m_height - 1));

    // finds the integer coordinates of the bottom-left pixel of the 2x2 grid.
    int x0 = static_cast<int>(floor(px));
    int y0 = static_cast<int>(floor(py));

    // finds the integer coordinates of the top-right pixel.
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    // calculates the fractional distances from the bottom-left pixel.
    // equation: dx = px - x0
    double dx = px - x0;
    // equation: dy = py - y0
    double dy = py - y0;

    // wraps the right-side x-coordinate for seamless horizontal interpolation.
    x1 = x1 % m_width;
    if (x1 < 0) x1 += m_width;

    // clamps the top-side y-coordinate to the image boundary.
    y1 = std::min(y1, m_height - 1);

    // a lambda function to get the rgb vector for a given pixel coordinate.
    auto get_pixel = [&](int x, int y) -> Vector3 {
        // calculates the index for the start of the pixel data in the 1d vector.
        // 'size_t' is an unsigned integer type used to represent the size of objects in bytes.
        size_t idx = (y * m_width + x) * 3;
        return Vector3(m_data[idx], m_data[idx+1], m_data[idx+2]);
    };

    // gets the four corner pixels surrounding the target coordinate.
    Vector3 c00 = get_pixel(x0, y0); // Bottom-Left
    Vector3 c10 = get_pixel(x1, y0); // Bottom-Right
    Vector3 c01 = get_pixel(x0, y1); // Top-Left
    Vector3 c11 = get_pixel(x1, y1); // Top-Right

    // linearly interpolates between the bottom two pixels.
    // equation: bottom = c00 * (1 - dx) + c10 * dx
    Vector3 bottom = c00 * (1.0 - dx) + c10 * dx;
    // linearly interpolates between the top two pixels.
    // equation: top = c01 * (1 - dx) + c11 * dx
    Vector3 top    = c01 * (1.0 - dx) + c11 * dx;

    // linearly interpolates vertically between the top and bottom interpolated results.
    // equation: result = bottom * (1 - dy) + top * dy
    return bottom * (1.0 - dy) + top * dy;

}