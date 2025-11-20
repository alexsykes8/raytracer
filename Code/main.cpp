#include "Image.h"
#include "ray.h"
#include "scene.h"
#include <iostream>
#include <string>
#include <vector>
#include "material.h"
#include "tracer.h"
#include <stdexcept>
#include "random_utils.h"
#include "config.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <atomic>

// This will only include the non-standard c library if it's compiled with OpenMP.
#ifdef _OPENMP
    #include <omp.h>
#endif

namespace fs = std::filesystem;

// for testing
std::string get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d_%H-%M-%S");
    return ss.str();
}

int main(int argc, char* argv[]) {
    Config::Instance().load("../config.json");

    // turning on and off features
    bool use_bvh = Config::Instance().getBool("render.use_bvh", true);
    int samples_per_pixel = Config::Instance().getInt("settings.samples_per_pixel", 1);
    double exposure = Config::Instance().getDouble("image.exposure", 1.0);
    bool enable_shadows = true;
    int glossy_samples = Config::Instance().getInt("render.glossy_samples", 0);
    bool enable_parallel = Config::Instance().getBool("render.parallel", false);
    double shutter_time = Config::Instance().getDouble("image.shutter_time", 0.0);
    bool enable_fresnel = false;
    bool any_hit_enabled = Config::Instance().getBool("render.any_hit_enabled", true);
    int run_count = 1;
    bool enable_timing = false;
    std::string all_args = "";

    // for logging
    for (int i = 1; i < argc; ++i) {
        all_args += std::string(argv[i]) + " ";
    }

    for (int i = 1; i<argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--no-bvh") {
            use_bvh = false;
            std::cout << "BVH disabled" << std::endl;
            break;
        }
        else if (arg == "--time") {
            if (i + 1 < argc) {
                try {
                    run_count = std::stoi(argv[i + 1]);
                    if (run_count < 1) run_count = 1;
                    enable_timing = true;
                    i++;
                    std::cout << "Timing enabled: " << run_count << " runs." << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error: Invalid value for --time flag. Must be an integer." << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Error: --time flag requires a number of runs." << std::endl;
                return 1;
            }
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
        else if (arg == "--fresnel") {
            enable_fresnel = true;
            std::cout << "Fresnel effect enabled" << std::endl;
        }

        else if (arg == "--any_hit_off") {
            any_hit_enabled = false;
            std::cout << "Any hit optimisation turned off." << std::endl;
        }

    }

    std::string timestamp_str;
    std::string output_dir;
    if (enable_timing) {
        timestamp_str = get_current_timestamp();
        output_dir = "../../Output/testing/" + timestamp_str;
        try {
            fs::create_directories(output_dir);
            std::cout << "Output directory created: " << output_dir << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error creating output directory: " << e.what() << std::endl;
            return 1;
        }
    }

    struct RunStats {
        double duration;
        std::string output_path;
    };
    std::vector<RunStats> timing_results;

    try {
        for (int i = 0; i < run_count; ++i) {
            if (enable_timing) {
                std::cout << "\n--- Starting Run " << (i + 1) << " of " << run_count << " ---" << std::endl;
            }

            auto start_time = std::chrono::high_resolution_clock::now();

            std::cout << "Loading scene..." << std::endl;
            const std::string scene_file = "../../ASCII/scene.txt";

            // initialises a scene. Prepares the objects, materials, and object matrices in preparation for calculations.
            Scene scene(scene_file, use_bvh, exposure, enable_shadows, glossy_samples, shutter_time, enable_fresnel, any_hit_enabled);

            const Camera& camera = scene.getCamera();
            const HittableList& world = scene.getWorld();

            const int width = camera.getResolutionX();
            const int height = camera.getResolutionY();
            Image image(width, height);

            // samples_per_pixel controls how many individual rays are cast into the scene for each individual pizel.
            // by default it is 1, where only 1 ray determines the colour of the pizel.
            const int SAMPLES_PER_PIXEL = samples_per_pixel;

            const int MAX_DEPTH = Config::Instance().getInt("settings.max_bounces", 10);

            std::cout << "Rendering scene (" << width << "x" << height << ") with "
                      << SAMPLES_PER_PIXEL << " samples per pixel..." << std::endl;

            int num_threads = 1;

            // if possible and if --parallel flag is present, use parallel flags.
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
            std::atomic<int> scanlines_completed(0);
            int total_scanlines = height;

            #ifdef _OPENMP
            #pragma omp parallel for schedule(dynamic, 10)
            #endif

            for (int y = 0; y < height; ++y) {
                if (y % (height / 20) == 0) {
                    #ifdef _OPENMP

                    #else
                                        std::cout << "Scanlines remaining: " << (height - y) << std::endl;
                    #endif
                }
                // for every pixel in the scene
                for (int x = 0; x < width; ++x) {
                    // initialises the colour of the pixel.
                    Vector3 pixel_color_vec(0, 0, 0);

                    // takes one or more pixel per sample, with a tiny bit of noise added so that each ray hits a slightly different area within the pixel.
                    for (int s = 0; s < SAMPLES_PER_PIXEL; ++s) {

                        float random_u = random_double();
                        float random_v = random_double();

                        float px = (static_cast<float>(x) + random_u) / width;
                        float py = (static_cast<float>(y) + random_v) / height;

                        double ray_time = random_double() * scene.get_shutter_time();

                        // generate a ray for the pixel. Defines the origin, direction and time for that particular pixel.
                        Ray ray = camera.generateRay(px, py, ray_time);

                        // decides the colour of the ray.
                        pixel_color_vec = pixel_color_vec + ray_colour(ray, scene, world, MAX_DEPTH);
                    }
                    // finds the average colour of the samples taken.
                    Vector3 averaged_color_vec = pixel_color_vec * (1.0 / SAMPLES_PER_PIXEL);
                    Pixel final_color = final_colour_to_pixel(averaged_color_vec);
                    image.setPixel(x, y, final_color);

                }
                int completed = ++scanlines_completed;
                if (completed % (total_scanlines / 20) == 0 || completed == total_scanlines) {
                    int percent = (static_cast<long long>(completed) * 100) / total_scanlines;
                    std::stringstream ss;
                    ss << "\rRendering: " << percent << "% [" << completed << "/" << total_scanlines << "]";
                    if (completed == total_scanlines) ss << std::endl;
                    std::cout << ss.str() << std::flush;
                }
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end_time - start_time;

            std::string output_file;
            if (enable_timing) {
                output_file = output_dir + "/output_" + timestamp_str + "_" + std::to_string(i + 1) + ".ppm";
            } else {
                output_file = "../../Output/scene_test.ppm";
            }

            image.write(output_file);

            if (enable_timing) {
                std::cout << "Run " << (i + 1) << " completed in " << elapsed.count() << " seconds." << std::endl;
                timing_results.push_back({elapsed.count(), output_file});
            } else {
                std::cout << "Render complete! Image saved to '" << output_file << "'." << std::endl;
            }
        }
        if (enable_timing) {
            std::string log_path = output_dir + "/timing_log.txt";
            std::ofstream log_file(log_path);
            if (log_file.is_open()) {
                log_file << "args: [" << all_args << "]\n";
                for (const auto& res : timing_results) {
                    log_file << "[" << res.duration << ", " << res.output_path << "]\n";
                }
                log_file.close();
                std::cout << "Timing log saved to: " << log_path << std::endl;
            } else {
                std::cerr << "Failed to write log file to: " << log_path << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}