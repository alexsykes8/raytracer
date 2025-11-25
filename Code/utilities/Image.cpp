#include "Image.h"
#include <fstream>
#include <iostream>
#include <cmath>

Pixel Image::getPixelBilinear(double u, double v) const {
    u = std::max(0.0, std::min(1.0, u));
    v = std::max(0.0, std::min(1.0, v));

    double px = u * (m_width - 1);
    double py = v * (m_height - 1);

    int x0 = static_cast<int>(std::floor(px));
    int y0 = static_cast<int>(std::floor(py));

    int x1 = std::min(x0 + 1, m_width - 1);
    int y1 = std::min(y0 + 1, m_height - 1);

    double dx = px - x0;
    double dy = py - y0;

    Pixel c00 = getPixel(x0, y0);
    Pixel c10 = getPixel(x1, y0);
    Pixel c01 = getPixel(x0, y1);
    Pixel c11 = getPixel(x1, y1);

    double r_top = (1.0 - dx) * c00.r + dx * c10.r;
    double g_top = (1.0 - dx) * c00.g + dx * c10.g;
    double b_top = (1.0 - dx) * c00.b + dx * c10.b;

    double r_bot = (1.0 - dx) * c01.r + dx * c11.r;
    double g_bot = (1.0 - dx) * c01.g + dx * c11.g;
    double b_bot = (1.0 - dx) * c01.b + dx * c11.b;

    unsigned char r = static_cast<unsigned char>((1.0 - dy) * r_top + dy * r_bot);
    unsigned char g = static_cast<unsigned char>((1.0 - dy) * g_top + dy * g_bot);
    unsigned char b = static_cast<unsigned char>((1.0 - dy) * b_top + dy * b_bot);

    return {r, g, b};
}

// Constructor to read an image from a file.
Image::Image(const std::string& filename) : m_width(0), m_height(0), m_max_color_val(255) {
    read(filename);
}

// Constructor to create a blank image.
Image::Image(int width, int height) : m_width(width), m_height(height), m_max_color_val(255) {
    if (width <= 0 || height <= 0) {
        throw std::runtime_error("Image width and height must be positive.");
    }
    // Allocate memory for the pixel data and initialize to black.
    m_pixel_data.resize(static_cast<size_t>(m_width) * m_height * 3, 0);
}

// Reads the PPM file data.
void Image::read(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::string magic_number;

    // reads the first piece of information from the image file and stores it
    file >> magic_number;

    if (magic_number != "P6" && magic_number != "P3") {
        throw std::runtime_error("Invalid PPM file format. Expected P6 (Binary) or P3 (ASCII).");
    }

    // Skip comments
    char peek_char = file.peek();
    while (file && (peek_char == '\n' || peek_char == '\r' || peek_char == ' ' || peek_char == '\t')) {
        file.get();
        peek_char = file.peek();
    }
    if (file && peek_char == '#') {
        std::string comment;
        std::getline(file, comment);
        // skip whitespace
        peek_char = file.peek();
        while (file && (peek_char == '\n' || peek_char == '\r' || peek_char == ' ' || peek_char == '\t')) {
            file.get();
            peek_char = file.peek();
        }
    }

    // Read image dimensions and max color value from the image header
    file >> m_width >> m_height >> m_max_color_val;

    if (m_max_color_val != 255) {
        throw std::runtime_error("Unsupported max color value. Only 255 is supported.");
    }

    // A single whitespace character separates the header from the pixel data.
    // .get() extracts and discards the single whitespace character that separates the header from pixel data.
    file.get();

    // resize the m_pixel_data vector to be large enough for all the pixels. * 3 is for the three colour components R G B
    size_t total_pixels = static_cast<size_t>(m_width) * m_height;
    m_pixel_data.resize(total_pixels * 3);

    if (magic_number == "P6") {
        // --- P6 (Binary) Reading ---
        file.read(reinterpret_cast<char*>(m_pixel_data.data()), m_pixel_data.size());

        if (!file) {
            if (file.eof()) {
                throw std::runtime_error("Error reading pixel data from file: " + filename + ". File ended unexpectedly.");
            } else {
                throw std::runtime_error("Error reading pixel data from file: " + filename + ".");
            }
        }
    } else if (magic_number == "P3") {
        // --- P3 (ASCII) Reading ---
        for (size_t i = 0; i < total_pixels * 3; ++i) {
            int color_value;
            if (!(file >> color_value)) {
                throw std::runtime_error("Error reading pixel data from P3 file: " + filename + ". Expected more color values.");
            }
            // clamp the colour value just in case
            if (color_value < 0 || color_value > 255) {
            }
            m_pixel_data[i] = static_cast<unsigned char>(color_value);
        }

        // final check for failure
        if (!file.eof() && file.fail() && !file.bad()) {
            throw std::runtime_error("Unexpected error during P3 pixel reading: " + filename);
        }
    }
}

// Writes the image data to a PPM file.
void Image::write(const std::string& filename) const {
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
Pixel Image::getPixel(int x, int y) const {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        throw std::out_of_range("Pixel coordinates are out of bounds.");
    }
    size_t index = (static_cast<size_t>(y) * m_width + x) * 3;
    return {m_pixel_data[index], m_pixel_data[index + 1], m_pixel_data[index + 2]};
}

// Sets the pixel at a specific coordinate.
void Image::setPixel(int x, int y, const Pixel& p) {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        throw std::out_of_range("Pixel coordinates are out of bounds.");
    }
    size_t index = (static_cast<size_t>(y) * m_width + x) * 3;
    m_pixel_data[index] = p.r;
    m_pixel_data[index + 1] = p.g;
    m_pixel_data[index + 2] = p.b;
}
