#include "Image.h"
#include "ray.h"
#include "scene.h"
#include <iostream>
#include <string>
#include <vector>
#include "material.h"
#include "shading.h"
#include "tracer.h"




int main(int argc, char* argv[]) {
    bool use_bvh = true;
    double exposure = 1.0;
    bool enable_shadows = false;

    for (int i = 1; i<argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--no-bvh") {
            use_bvh = false;
            std::cout << "BVH disabled" << std::endl;
        }
        else if (arg == "--exposure") {
            if (i + 1 < argc) {
                try {
                    exposure = std::stod(argv[i + 1]);
                    i++;
                    std::cout << "Exposure set to: " << exposure << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error: Invalid value for --exposure flag." << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Error: --exposure flag requires a value (e.g., --exposure 0.5)." << std::endl;
                return 1;
            }
        }
        else if (arg == "--shadows") {
            enable_shadows = true;
            std::cout << "Shadows enabled" << std::endl;
        }
    }
    try {
        std::cout << "Loading scene..." << std::endl;
        const std::string scene_file = "../../ASCII/scene.txt";

        Scene scene(scene_file, use_bvh, exposure, enable_shadows);

        const Camera& camera = scene.getCamera();
        const HittableList& world = scene.getWorld();

        const int width = camera.getResolutionX();
        const int height = camera.getResolutionY();
        Image image(width, height);

        std::cout << "Rendering scene (" << width << "x" << height << ")..." << std::endl;

        for (int y = 0; y < height; ++y) {
            if (y % (height / 10) == 0) {
                std::cout << "Scanlines remaining: " << (height - y) << std::endl;
            }
            for (int x = 0; x < width; ++x) {
                float px = (static_cast<float>(x) + 0.5f) / width;
                float py = (static_cast<float>(y) + 0.5f) / height;

                Ray ray = camera.generateRay(px, py);
                Vector3 final_color_vec = ray_colour(ray, scene, world, MAX_RECURSION_DEPTH);
                Pixel final_pixel = final_colour_to_pixel(final_color_vec);
                image.setPixel(x, y, final_pixel);
            }
        }

        const std::string output_file = "../../Output/scene_test.ppm";
        image.write(output_file);
        std::cout << "Render complete! Image saved to '" << output_file << "'." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

