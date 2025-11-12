#include "Image.h"
#include "ray.h"
#include "scene.h"
#include <iostream>
#include <string>
#include <vector>
#include "material.h"
#include "tracer.h"
#include <stdexcept>

// This will only include the non-standard c library if its compiled with OpenMP. This is a post-spec feature
// to improve runtime by using parallel threads.
#ifdef _OPENMP
    #include <omp.h>
#endif



int main(int argc, char* argv[]) {
    // random number for antialiasing
    srand(static_cast<unsigned int>(time(nullptr)));

    bool use_bvh = true;
    int samples_per_pixel = 1;
    double exposure = 1.0;
    bool enable_shadows = false;
    int glossy_samples = 0;
    bool enable_parallel = false;
    double shutter_time = 0.0;

    for (int i = 1; i<argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--no-bvh") {
            use_bvh = false;
            std::cout << "BVH disabled" << std::endl;
            break;
        }
        if (arg == "--aa") {
            if (i + 1 < argc) {
                try {
                    samples_per_pixel = std::stoi(argv[i + 1]);
                    i++;
                    std::cout << "Antialiasing enabled: " << samples_per_pixel << " samples/pixel." << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error: Invalid value for --aa flag. Must be an integer." << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Error: --aa flag requires a number of samples (e.g., --aa 16)." << std::endl;
                return 1;
            }
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

        else if (arg == "--glossy") {
            if (i + 1 < argc) {
                try {
                    glossy_samples = std::stoi(argv[i + 1]);
                    i++;
                    std::cout << "Glossy reflections enabled: " << glossy_samples << " samples." << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error: Invalid value for --glossy flag. Must be an integer." << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Error: --glossy flag requires a number of samples (e.g., --glossy 16)." << std::endl;
                return 1;
            }
        }

        else if (arg == "--parallel") {
            enable_parallel = true;
            std::cout << "Parallel rendering enabled" << std::endl;
        }

        else if (arg == "--motion-blur") {
            if (i + 1 < argc) {
                try {
                    // takes a double via stod
                    shutter_time = std::stod(argv[i + 1]);
                    i++;
                    std::cout << "Motion blur enabled. Shutter time: " << shutter_time << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error: Invalid value for --motion-blur flag." << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Error: --motion-blur requires a time value (e.g., --motion-blur 1.0)." << std::endl;
                return 1;
            }
        }

    }
    try {
        std::cout << "Loading scene..." << std::endl;
        const std::string scene_file = "../../ASCII/scene.txt";

        Scene scene(scene_file, use_bvh, exposure, enable_shadows, glossy_samples, shutter_time);

        const Camera& camera = scene.getCamera();
        const HittableList& world = scene.getWorld();

        const int width = camera.getResolutionX();
        const int height = camera.getResolutionY();
        Image image(width, height);

        const int SAMPLES_PER_PIXEL = samples_per_pixel;

        std::cout << "Rendering scene (" << width << "x" << height << ") with "
                  << SAMPLES_PER_PIXEL << " samples per pixel..." << std::endl;

        int num_threads = 1;
        #ifdef _OPENMP
                if (!enable_parallel) {
                    omp_set_num_threads(1);
                }
                num_threads = omp_get_max_threads();
        #else
                if (enable_parallel) {
                    std::cout << "Warning: --parallel flag ignored. Program was not compiled with OpenMP." << std::endl;
                }
        #endif

        std::cout << "Starting render with " << num_threads << " thread(s)..." << std::endl;
        #ifdef _OPENMP
        #pragma omp parallel for
        #endif

        for (int y = 0; y < height; ++y) {
            if (y % (height / 20) == 0) {
                #ifdef _OPENMP
                                std::cout << "Scanlines remaining: " << (height - y) << " (Thread " << omp_get_thread_num() << ")" << std::endl;
                #else
                                std::cout << "Scanlines remaining: " << (height - y) << std::endl;
                #endif
            }
            for (int x = 0; x < width; ++x) {
                Vector3 pixel_color_vec(0, 0, 0);

                for (int s = 0; s < SAMPLES_PER_PIXEL; ++s) {

                    float random_u = static_cast<float>(rand()) / (RAND_MAX + 1.0f);
                    float random_v = static_cast<float>(rand()) / (RAND_MAX + 1.0f);

                    float px = (static_cast<float>(x) + random_u) / width;
                    float py = (static_cast<float>(y) + random_v) / height;

                    double ray_time = random_double() * scene.get_shutter_time();

                    Ray ray = camera.generateRay(px, py, ray_time);

                    pixel_color_vec = pixel_color_vec + ray_colour(ray, scene, world, MAX_RECURSION_DEPTH);
                }
                Vector3 averaged_color_vec = pixel_color_vec * (1.0 / SAMPLES_PER_PIXEL);
                Pixel final_color = final_colour_to_pixel(averaged_color_vec);
                image.setPixel(x, y, final_color);

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

