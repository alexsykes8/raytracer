//
// Created by alex on 08/10/2025.
//

#ifndef B216602_IMAGE_H
#define B216602_IMAGE_H


#include <string>
#include <vector>
#include <stdexcept>

// A simple struct to represent a single RGB pixel.
struct Pixel {
    unsigned char r, g, b;
};

/**
 * A class to handle reading, writing, and modifying binary PPM (P6) image files.
 */
class Image {
public:
    // take a file name to read from
    explicit Image(const std::string& filename);

    // create a blank image of a given size.
    Image(int width, int height);

    // write the image to a specified file.
    void write(const std::string& filename) const;

    // Getters used to read & modify pixel values.
    Pixel getPixel(int x, int y) const;
    void setPixel(int x, int y, const Pixel& p);

    // Getters for image dimensions.
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

    Pixel getPixelBilinear(double u, double v) const;

private:
    // Helper method to read the PPM file
    void read(const std::string& filename);

    int m_width;
    int m_height;
    int m_max_color_val;
    // Pixel data is stored as a flat 1D vector of bytes (R,G,B,R,G,B,...).
    std::vector<unsigned char> m_pixel_data;
};

#endif //B216602_IMAGE_H