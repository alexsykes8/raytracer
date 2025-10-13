#include "image.h"
#include <fstream>
#include <iostream>

// Constructor to read an image from a file.
PPMImage::PPMImage(const std::string& filename) : m_width(0), m_height(0), m_max_color_val(255) {
    read(filename);
}

// Constructor to create a blank image.
PPMImage::PPMImage(int width, int height) : m_width(width), m_height(height), m_max_color_val(255) {
    if (width <= 0 || height <= 0) {
        throw std::runtime_error("Image width and height must be positive.");
    }
    // Allocate memory for the pixel data and initialize to black.
    m_pixel_data.resize(static_cast<size_t>(m_width) * m_height * 3, 0);
}

// Reads the PPM file data.
void PPMImage::read(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::string magic_number;
    file >> magic_number;

    if (magic_number != "P6") {
        throw std::runtime_error("Invalid PPM file format. Expected P6 binary.");
    }

    // Skip comments
    char peek_char = file.peek();
    while (peek_char == '\n' || peek_char == '\r') {
        file.get();
        peek_char = file.peek();
    }
    if (peek_char == '#') {
        std::string comment;
        std::getline(file, comment);
    }

    file >> m_width >> m_height >> m_max_color_val;

    if (m_max_color_val != 255) {
        throw std::runtime_error("Unsupported max color value. Only 255 is supported.");
    }

    // A single whitespace character separates the header from the pixel data.
    file.get();

    m_pixel_data.resize(static_cast<size_t>(m_width) * m_height * 3);
    file.read(reinterpret_cast<char*>(m_pixel_data.data()), m_pixel_data.size());

    if (!file) {
        throw std::runtime_error("Error reading pixel data from file: " + filename);
    }
}

// Writes the image data to a PPM file.
void PPMImage::write(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }

    // Write PPM header
    file << "P6\n";
    file << m_width << " " << m_height << "\n";
    file << m_max_color_val << "\n";

    // Write pixel data
    file.write(reinterpret_cast<const char*>(m_pixel_data.data()), m_pixel_data.size());

    if (!file) {
        throw std::runtime_error("Error writing pixel data to file: " + filename);
    }
}

// Gets the pixel at a specific coordinate.
Pixel PPMImage::getPixel(int x, int y) const {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        throw std::out_of_range("Pixel coordinates are out of bounds.");
    }
    size_t index = (static_cast<size_t>(y) * m_width + x) * 3;
    return {m_pixel_data[index], m_pixel_data[index + 1], m_pixel_data[index + 2]};
}

// Sets the pixel at a specific coordinate.
void PPMImage::setPixel(int x, int y, const Pixel& p) {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        throw std::out_of_range("Pixel coordinates are out of bounds.");
    }
    size_t index = (static_cast<size_t>(y) * m_width + x) * 3;
    m_pixel_data[index] = p.r;
    m_pixel_data[index + 1] = p.g;
    m_pixel_data[index + 2] = p.b;
}
