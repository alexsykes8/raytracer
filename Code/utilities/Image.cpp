#include "Image.h"
#include <fstream>
#include <iostream>
#include <cmath>

// performs bilinear interpolation to get a smooth pixel colour from normalised (u,v) coordinates.
Pixel Image::getPixelBilinear(double u, double v) const {
    // clamps the u and v coordinates to the valid [0, 1] range.
    u = std::max(0.0, std::min(1.0, u));
    v = std::max(0.0, std::min(1.0, v));

    // converts normalised coordinates to floating-point pixel coordinates.
    // equation: px = u * (width - 1)
    double px = u * (m_width - 1);
    // equation: py = v * (height - 1)
    double py = v * (m_height - 1);

    // finds the integer coordinates of the top-left pixel of the 2x2 grid.
    // 'static_cast' performs an explicit type conversion at compile time.
    int x0 = static_cast<int>(std::floor(px));
    int y0 = static_cast<int>(std::floor(py));

    // finds the integer coordinates of the bottom-right pixel, clamping to image boundaries.
    int x1 = std::min(x0 + 1, m_width - 1);
    int y1 = std::min(y0 + 1, m_height - 1);

    // calculates the fractional distances from the top-left pixel.
    // equation: dx = px - x0
    double dx = px - x0;
    // equation: dy = py - y0
    double dy = py - y0;

    // gets the four corner pixels surrounding the target coordinate.
    Pixel c00 = getPixel(x0, y0);
    Pixel c10 = getPixel(x1, y0);
    Pixel c01 = getPixel(x0, y1);
    Pixel c11 = getPixel(x1, y1);

    // linearly interpolates between the top two pixels for each colour channel.
    // equation: r_top = (1.0 - dx) * c00.r + dx * c10.r
    double r_top = (1.0 - dx) * c00.r + dx * c10.r;
    // equation: g_top = (1.0 - dx) * c00.g + dx * c10.g
    double g_top = (1.0 - dx) * c00.g + dx * c10.g;
    // equation: b_top = (1.0 - dx) * c00.b + dx * c10.b
    double b_top = (1.0 - dx) * c00.b + dx * c10.b;

    // linearly interpolates between the bottom two pixels for each colour channel.
    // equation: r_bot = (1.0 - dx) * c01.r + dx * c11.r
    double r_bot = (1.0 - dx) * c01.r + dx * c11.r;
    // equation: g_bot = (1.0 - dx) * c01.g + dx * c11.g
    double g_bot = (1.0 - dx) * c01.g + dx * c11.g;
    // equation: b_bot = (1.0 - dx) * c01.b + dx * c11.b
    double b_bot = (1.0 - dx) * c01.b + dx * c11.b;

    // linearly interpolates vertically between the top and bottom interpolated results.
    // equation: r = (1.0 - dy) * r_top + dy * r_bot
    unsigned char r = static_cast<unsigned char>((1.0 - dy) * r_top + dy * r_bot);
    // equation: g = (1.0 - dy) * g_top + dy * g_bot
    unsigned char g = static_cast<unsigned char>((1.0 - dy) * g_top + dy * g_bot);
    // equation: b = (1.0 - dy) * b_top + dy * b_bot
    unsigned char b = static_cast<unsigned char>((1.0 - dy) * b_top + dy * b_bot);

    // returns the final interpolated pixel.
    return {r, g, b};
}

// constructor to read an image from a file.
Image::Image(const std::string& filename) : m_width(0), m_height(0), m_max_color_val(255) {
    read(filename);
}

// constructor to create a blank image.
Image::Image(int width, int height) : m_width(width), m_height(height), m_max_color_val(255) {
    // validates that the image dimensions are positive.
    if (width <= 0 || height <= 0) {
        throw std::runtime_error("Image width and height must be positive.");
    }
    // allocates memory for the pixel data and initialises it to black (0).
    // 'size_t' is an unsigned integer type used to represent the size of objects in bytes.
    m_pixel_data.resize(static_cast<size_t>(m_width) * m_height * 3, 0);
}

// reads the ppm file data.
void Image::read(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    // a string to store the ppm magic number (e.g., "p3" or "p6").
    std::string magic_number;

    // reads the magic number from the file header.
    file >> magic_number;

    // validates the ppm format. only p3 (ascii) and p6 (binary) are supported.
    if (magic_number != "P6" && magic_number != "P3") {
        throw std::runtime_error("Invalid PPM file format. Expected P6 (Binary) or P3 (ASCII).");
    }

    // skips any whitespace characters after the magic number.
    char peek_char = file.peek();
    while (file && (peek_char == '\n' || peek_char == '\r' || peek_char == ' ' || peek_char == '\t')) {
        file.get();
        peek_char = file.peek();
    }
    // skips any comment lines, which start with '#'.
    if (file && peek_char == '#') {
        std::string comment;
        std::getline(file, comment);
        // skips any whitespace after the comment.
        peek_char = file.peek();
        while (file && (peek_char == '\n' || peek_char == '\r' || peek_char == ' ' || peek_char == '\t')) {
            file.get();
            peek_char = file.peek();
        }
    }

    // reads image dimensions and max colour value from the image header.
    file >> m_width >> m_height >> m_max_color_val;

    // ensures the max colour value is 255, as other values are not supported.
    if (m_max_color_val != 255) {
        throw std::runtime_error("Unsupported max color value. Only 255 is supported.");
    }

    // extracts and discards the single whitespace character that separates the header from the pixel data.
    file.get();

    // resizes the pixel data vector to hold all pixel data (width * height * 3 bytes for rgb).
    size_t total_pixels = static_cast<size_t>(m_width) * m_height;
    m_pixel_data.resize(total_pixels * 3);

    if (magic_number == "P6") {
        // reads the entire block of binary pixel data at once.
        // 'reinterpret_cast' converts a pointer to any other pointer type, used here to treat the byte vector as a char array for reading.
        file.read(reinterpret_cast<char*>(m_pixel_data.data()), m_pixel_data.size());

        // checks for read errors.
        if (!file) {
            if (file.eof()) {
                throw std::runtime_error("Error reading pixel data from file: " + filename + ". File ended unexpectedly.");
            } else {
                throw std::runtime_error("Error reading pixel data from file: " + filename + ".");
            }
        }
    } else if (magic_number == "P3") {
        // reads ascii pixel data value by value.
        for (size_t i = 0; i < total_pixels * 3; ++i) {
            int color_value;
            // reads a single integer colour value.
            if (!(file >> color_value)) {
                throw std::runtime_error("Error reading pixel data from P3 file: " + filename + ". Expected more color values.");
            }
            // an empty block that would have clamped the colour value, but is currently unused.
            if (color_value < 0 || color_value > 255) {
            }
            // stores the integer value as an unsigned char in the pixel data vector.
            m_pixel_data[i] = static_cast<unsigned char>(color_value);
        }

        // performs a final check for any read failures.
        if (!file.eof() && file.fail() && !file.bad()) {
            throw std::runtime_error("Unexpected error during P3 pixel reading: " + filename);
        }
    }
}

// writes the image data to a ppm file.
void Image::write(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }

    // Write PPM header
    file << "P6\n";
    file << m_width << " " << m_height << "\n";
    file << m_max_color_val << "\n";

    // writes the entire block of pixel data to the file.
    file.write(reinterpret_cast<const char*>(m_pixel_data.data()), m_pixel_data.size());

    // checks for write errors.
    if (!file) {
        throw std::runtime_error("Error writing pixel data to file: " + filename);
    }
}

// gets the pixel at a specific coordinate.
Pixel Image::getPixel(int x, int y) const {
    // checks if the requested coordinates are within the image bounds.
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        throw std::out_of_range("Pixel coordinates are out of bounds.");
    }
    // calculates the index for the start of the pixel data in the 1d vector.
    // equation: index = (y * width + x) * 3
    size_t index = (static_cast<size_t>(y) * m_width + x) * 3;
    return {m_pixel_data[index], m_pixel_data[index + 1], m_pixel_data[index + 2]};
}

// sets the pixel at a specific coordinate.
void Image::setPixel(int x, int y, const Pixel& p) {
    // checks if the target coordinates are within the image bounds.
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        throw std::out_of_range("Pixel coordinates are out of bounds.");
    }
    // calculates the index for the start of the pixel data in the 1d vector.
    size_t index = (static_cast<size_t>(y) * m_width + x) * 3;
    m_pixel_data[index] = p.r;
    m_pixel_data[index + 1] = p.g;
    m_pixel_data[index + 2] = p.b;
}
