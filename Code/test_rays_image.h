//
// Created by alex on 16/10/2025.
//

#ifndef B216602_TEST_RAYS_IMAGE_H
#define B216602_TEST_RAYS_IMAGE_H



#include <string>
#include <vector>
#include <stdexcept>

// A simple struct to represent a pixel with RGB values.
struct Test_Pixel {
    unsigned char r, g, b;
};

/**
 * @brief A simple class to represent and manipulate an image.
 * Provides functionality to create, set pixels, and write to a PPM file.
 */
class Test_Image {
public:
    // Constructor to create a blank image of a given size.
    Test_Image(int width, int height);

    // Get the width of the image.
    int getWidth() const;

    // Get the height of the image.
    int getHeight() const;

    // Set the color of a specific pixel.
    void setPixel(int x, int y, const Test_Pixel& p);

    // Get the color of a specific pixel.
    Test_Pixel getPixel(int x, int y) const;

    // Write the image data to a PPM file.
    void write(const std::string& filename) const;

private:
    int m_width;
    int m_height;
    std::vector<Test_Pixel> m_data; // Image data stored as a 1D vector.
};

#endif //B216602_TEST_RAYS_IMAGE_H