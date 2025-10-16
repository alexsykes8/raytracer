#include "Image.h"
#include "camera.h"
#include "ray.h"
#include <iostream>
#include <vector>
#include <random>
#include <iomanip>
#include <fstream>

#include "test_rays_image.h"

int test_ray_generation() {
    const int width = 700;
    const int height = 700;
    const int num_samples = 50;
    const std::string scene_file = "../ASCII/scene.txt";
    const std::string output_image_file = "../Output/ray_samples.ppm";
    const std::string output_ray_file = "../Output/rays.txt";

    try {
        // 1. Initialize the camera from the scene file.
        std::cout << "Initializing camera from '" << scene_file << "'..." << std::endl;
        Camera camera(scene_file);

        // 2. Create a 700x700 image and fill it with white.
        Test_Image image(width, height);
        Test_Pixel white = {255, 255, 255};
        Test_Pixel red = {255, 0, 0};
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                image.setPixel(x, y, white);
            }
        }

        // Prepare to store rays and generate random pixel coordinates.
        std::vector<Ray> sampled_rays;
        sampled_rays.reserve(num_samples);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distribX(0, width - 1);
        std::uniform_int_distribution<> distribY(0, height - 1);

        std::cout << "Generating " << num_samples << " random ray samples..." << std::endl;

        // 3. Uniformly sample pixels, generate rays, and mark pixels red.
        for (int i = 0; i < num_samples; ++i) {
            int x = distribX(gen);
            int y = distribY(gen);

            // Normalize pixel coordinates to [0, 1] for generateRay.
            // Add 0.5 to sample from the center of the pixel.
            float px = (static_cast<float>(x) + 0.5f) / width;
            float py = (static_cast<float>(y) + 0.5f) / height;

            // Generate and store the ray.
            Ray ray = camera.generateRay(px, py);
            sampled_rays.push_back(ray);

            // Mark the sampled pixel in red.
            image.setPixel(x, y, red);
        }

        // 4. Write the image with red sample points to a file.
        image.write(output_image_file);
        std::cout << "Successfully wrote sample visualization to '" << output_image_file << "'." << std::endl;

        // 5. Write the list of rays to a text file.
        std::ofstream ray_file(output_ray_file);
        if (!ray_file.is_open()) {
            throw std::runtime_error("Failed to open file for writing rays: " + output_ray_file);
        }

        std::cout << "Writing sampled rays to '" << output_ray_file << "'..." << std::endl;
        ray_file << std::fixed << std::setprecision(6); // Set precision for output
        for (const auto& ray : sampled_rays) {
            ray_file << ray.origin.x << " " << ray.origin.y << " " << ray.origin.z << " "
                        << ray.direction.x << " " << ray.direction.y << " " << ray.direction.z << std::endl;
        }
        ray_file.close();
        std::cout << "Successfully wrote rays to file." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}


int test_image_class() {
    try {
        // 1. Create a new image programmatically.
        std::cout << "Creating a 256x256 image with a color gradient..." << std::endl;
        Image image(256, 256);

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
        const std::string original_filename = "../Output/gradient.ppm";
        std::cout << "Writing the gradient image to '" << original_filename << "'..." << std::endl;
        image.write(original_filename);

        // 3. Read the image back from the file using the main constructor.
        std::cout << "Reading image from '" << original_filename << "'..." << std::endl;
        Image loaded_image(original_filename);

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
        const std::string modified_filename = "../Output/modified.ppm";
        std::cout << "Writing the modified image to '" << modified_filename << "'..." << std::endl;
        loaded_image.write(modified_filename);

        std::cout << "\nProcess complete! Check the project directory for 'gradient.ppm' and 'modified.ppm'." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

int main() {
    try {
        return test_image_class();
    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

