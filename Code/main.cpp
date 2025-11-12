#include "Image.h"
#include "ray.h"
#include "scene.h"
#include <iostream>
#include <string>
#include <vector>
#include "material.h"
#include "shading.h"


Pixel color_from_normal(const Vector3& normal) {
    unsigned char r = static_cast<unsigned char>((normal.x + 1.0) * 0.5 * 255);
    unsigned char g = static_cast<unsigned char>((normal.y + 1.0) * 0.5 * 255);
    unsigned char b = static_cast<unsigned char>((normal.z + 1.0) * 0.5 * 255);
    return {r, g, b};
}



int main(int argc, char* argv[]) {
    bool use_bvh = true;

    for (int i = 1; i<argc; ++i) {
        if (std::string(argv[i]) == "--no-bvh") {
            use_bvh = false;
            std::cout << "BVH disabled" << std::endl;
            break;
        }
    }
    try {
        std::cout << "Loading scene..." << std::endl;
        const std::string scene_file = "../../ASCII/scene.txt";

        Scene scene(scene_file, use_bvh);

        const Camera& camera = scene.getCamera();
        const HittableList& world = scene.getWorld();

        const int width = camera.getResolutionX();
        const int height = camera.getResolutionY();
        Image image(width, height);
        Pixel background_color = {135, 206, 235};

        std::cout << "Rendering scene (" << width << "x" << height << ")..." << std::endl;

        for (int y = 0; y < height; ++y) {
            if (y % (height / 10) == 0) {
                std::cout << "Scanlines remaining: " << (height - y) << std::endl;
            }
            for (int x = 0; x < width; ++x) {
                float px = (static_cast<float>(x) + 0.5f) / width;
                float py = (static_cast<float>(y) + 0.5f) / height;

                Ray ray = camera.generateRay(px, py);
                HitRecord rec;

                if (world.intersect(ray, 0.001, 100000.0, rec)) {
                    Pixel final_color = blinn_phong_shade(rec, scene, ray);
                    image.setPixel(x, y, final_color);
                } else {
                    image.setPixel(x, y, background_color);
                }
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

