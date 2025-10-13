#include "image.h"
#include <iostream>

int main() {
    try {
        // 1. Create a new image programmatically.
        std::cout << "Creating a 256x256 image with a color gradient..." << std::endl;
        PPMImage image(256, 256);

        // Fill the image with a red-green gradient.
        for (int y = 0; y < image.getHeight(); ++y) {
            for (int x = 0; x < image.getWidth(); ++x) {
                Pixel p{};
                p.r = static_cast<unsigned char>(x); // Red changes along x-axis
                p.g = static_cast<unsigned char>(y); // Green changes along y-axis
                p.b = 0;
                image.setPixel(x, y, p);
            }
        }

        // 2. Write the new image to a file.
        const std::string original_filename = "gradient.ppm";
        std::cout << "Writing the gradient image to '" << original_filename << "'..." << std::endl;
        image.write(original_filename);

        // 3. Read the image back from the file using the main constructor.
        std::cout << "Reading image from '" << original_filename << "'..." << std::endl;
        PPMImage loaded_image(original_filename);

        // 4. Modify the pixel values of the loaded image.
        std::cout << "Modifying the image by drawing a white cross..." << std::endl;
        Pixel white = {255, 255, 255};
        // Draw a horizontal line
        for (int x = 0; x < loaded_image.getWidth(); ++x) {
            loaded_image.setPixel(x, loaded_image.getHeight() / 2, white);
        }
        // Draw a vertical line
        for (int y = 0; y < loaded_image.getHeight(); ++y) {
            loaded_image.setPixel(loaded_image.getWidth() / 2, y, white);
        }

        // 5. Write the modified image to a new file.
        const std::string modified_filename = "modified.ppm";
        std::cout << "Writing the modified image to '" << modified_filename << "'..." << std::endl;
        loaded_image.write(modified_filename);

        std::cout << "\nProcess complete! Check the project directory for 'gradient.ppm' and 'modified.ppm'." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}