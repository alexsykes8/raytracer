
#include "utilities/Image.h"
#include "utilities/ray.h"
#include "utilities/scene.h"
#include <iostream>
#include <string>
#include <vector>
#include "shapes/material.h"
#include "utilities/tracer.h"
#include <stdexcept>
#include "utilities/random_utils.h"
#include "config.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <atomic>
#include <unordered_map>
#include <functional>
#include <regex>
#include <algorithm>
#include <numeric>

// This will only include the non-standard c library if it's compiled with OpenMP.
#ifdef _OPENMP
    #include <omp.h>
#endif

namespace fs = std::filesystem;

// generates a timestamp string for file naming.
std::string get_current_timestamp() {
    // get current time point.
    auto now = std::chrono::system_clock::now();
    // convert time point to time_t.
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    // stringstream to build the formatted time string.
    std::stringstream ss;
    // format time_t to a string in YYYY-MM-DD_HH-MM-SS format.
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d_%H-%M-%S");
    return ss.str();
}

int main(int argc, char* argv[]) {
    // load configuration from a json file.
    Config::Instance().load("../config.json");

    // default feature flags and render settings.
    bool use_bvh = true;
    // get samples per pixel from config, default to 1.
    int samples_per_pixel = Config::Instance().getInt("settings.samples_per_pixel", 1);
    // get exposure from config, default to 1.0.
    double exposure = Config::Instance().getDouble("image.exposure", 1.0);
    bool enable_shadows = false;
    // get glossy samples from config, default to 0.
    int glossy_samples = Config::Instance().getInt("render.glossy_samples", 0);
    bool enable_parallel = false;
    // get shutter time from config, default to 0.0.
    double shutter_time = Config::Instance().getDouble("image.shutter_time", 0.0);
    bool enable_fresnel = false;
    int run_count = 1;
    bool enable_timing = false;
    bool render_normals = false;
    bool enable_bvh_testing = false;
    int tonemap_mode = 0; // 0=None, 1=Reinhard, 2=ACES, 3=Filmic
    std::string all_args = "";

    // concatenate all command line arguments for logging purposes.
    for (int i = 1; i < argc; ++i) {
        all_args += std::string(argv[i]) + " ";
    }

    // map to hold handlers for command-line arguments.
    // unordered map: dictionary of keys and values.
    // maps the command line argument with the handler (lines of code) that it should execute
    std::unordered_map<std::string, std::function<void(int&, int, char*[])>> arg_handlers;

    // handler for '--no-bvh' flag.
    arg_handlers["--no-bvh"] = [&](int& i, int argc, char* argv[]) {
        use_bvh = false;
        std::cout << "BVH disabled" << std::endl;
    };

    // handler for '--time' flag, which enables performance timing.
    arg_handlers["--time"] = [&](int& i, int argc, char* argv[]) {
        if (i + 1 < argc) {
            try {
                // convert the next argument to an integer for run count.
                run_count = std::stoi(argv[i + 1]);
                // ensure run count is at least 1.
                if (run_count < 1) run_count = 1;
                enable_timing = true;
                i++;
                std::cout << "Timing enabled: " << run_count << " runs." << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid value for --time flag. Must be an integer." << std::endl;
                exit(1);
            }
        } else {
            std::cerr << "Error: --time flag requires a number of runs." << std::endl;
            exit(1);
        }
    };

    // handler for '--aa' (anti-aliasing) flag.
    arg_handlers["--aa"] = [&](int& i, int argc, char* argv[]) {
        if (i + 1 < argc) {
            try {
                // convert the next argument to an integer for samples per pixel.
                samples_per_pixel = std::stoi(argv[i + 1]);
                i++;
                std::cout << "Antialiasing enabled: " << samples_per_pixel << " samples/pixel." << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid value for --aa flag. Must be an integer." << std::endl;
                exit(1);
            }
        } else {
            std::cerr << "Error: --aa flag requires a number of samples (e.g., --aa 16)." << std::endl;
            exit(1);
        }
    };

    // handler for '--exposure' flag.
    arg_handlers["--exposure"] = [&](int& i, int argc, char* argv[]) {
        if (i + 1 < argc) {
            try {
                // convert the next argument to a double for exposure value.
                exposure = std::stod(argv[i + 1]);
                i++;
                std::cout << "Exposure set to: " << exposure << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid value for --exposure flag." << std::endl;
                exit(1);
            }
        } else {
            std::cerr << "Error: --exposure flag requires a value (e.g., --exposure 0.5)." << std::endl;
            exit(1);
        }
    };

    // handler for '--shadows' flag.
    arg_handlers["--shadows"] = [&](int& i, int argc, char* argv[]) {
        enable_shadows = true;
        std::cout << "Shadows enabled" << std::endl;
    };

    // handler for '--glossy' flag.
    arg_handlers["--glossy"] = [&](int& i, int argc, char* argv[]) {
        std::cout << "Glossy flag present: using sample count from config.json" << std::endl;
    };

    // handler for '--parallel' flag.
    arg_handlers["--parallel"] = [&](int& i, int argc, char* argv[]) {
        enable_parallel = true;
        std::cout << "Parallel rendering enabled" << std::endl;
    };

    // handler for '--motion-blur' flag.
    arg_handlers["--motion-blur"] = [&](int& i, int argc, char* argv[]) {
        if (i + 1 < argc) {
            try {
                // convert the next argument to a double for shutter time.
                shutter_time = std::stod(argv[i + 1]);
                i++;
                std::cout << "Motion blur enabled. Shutter time: " << shutter_time << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid value for --motion-blur flag." << std::endl;
                exit(1);
            }
        } else {
            std::cerr << "Error: --motion-blur requires a time value (e.g., --motion-blur 1.0)." << std::endl;
            exit(1);
        }
    };

    // handler for '--fresnel' flag.
    arg_handlers["--fresnel"] = [&](int& i, int argc, char* argv[]) {
        enable_fresnel = true;
        std::cout << "Fresnel effect enabled" << std::endl;
    };

    arg_handlers["--normals"] = [&](int& i, int argc, char* argv[]) {
        render_normals = true;
        std::cout << "Debug: Rendering surface normals." << std::endl;
    };

    // handler for '--bvh_testing' flag.
    arg_handlers["--bvh_testing"] = [&](int& i, int argc, char* argv[]) {
        enable_bvh_testing = true;
        std::cout << "BVH testing mode enabled." << std::endl;
    };

    // handler for '--tonemap' flag.
    arg_handlers["--tonemap"] = [&](int& i, int argc, char* argv[]) {
        if (i + 1 < argc) {
            std::string mode = argv[i + 1];
            std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);

            if (mode == "reinhard") tonemap_mode = 1;
            else if (mode == "aces") tonemap_mode = 2;
            else if (mode == "filmic") tonemap_mode = 3;
            else {
                std::cerr << "Unknown tonemap mode: " << mode << " (defaulting to none)" << std::endl;
                tonemap_mode = 0;
            }
            i++;
            std::cout << "Tone mapping set to: " << mode << std::endl;
        } else {
            std::cerr << "Error: --tonemap requires a mode (reinhard, aces, filmic)." << std::endl;
            exit(1);
        }
    };

    // parse command-line arguments.
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        // check if a handler exists for the current argument.
        if (arg_handlers.count(arg)) {
            // call the handler.
            arg_handlers[arg](i, argc, argv);
        }
    }

    // Encapsulated rendering logic used by both standard and test modes
    auto render_scene_func = [&](const std::string& scene_path, bool current_use_bvh, const std::string& output_path) -> double {
        auto start_time = std::chrono::high_resolution_clock::now();

        std::cout << "Loading scene: " << scene_path << (current_use_bvh ? " [BVH ON]" : " [BVH OFF]") << std::endl;

        // initialises a scene. prepares the objects, materials, and object matrices in preparation for calculations.
        Scene scene(scene_path, current_use_bvh, exposure, enable_shadows, glossy_samples, shutter_time, enable_fresnel, render_normals);

        const Camera& camera = scene.getCamera();
        const HittableList& world = scene.getWorld();

        const int width = camera.getResolutionX();
        const int height = camera.getResolutionY();
        Image image(width, height);

        // samples_per_pixel controls how many individual rays are cast into the scene for each individual pixel.
        // by default it is 1, where only 1 ray determines the colour of the pixel.
        const int SAMPLES_PER_PIXEL = samples_per_pixel;

        // maximum number of ray bounces for path tracing.
        const int MAX_DEPTH = Config::Instance().getInt("settings.max_bounces", 10);

        std::cout << "Rendering scene (" << width << "x" << height << ") with "
                    << SAMPLES_PER_PIXEL << " samples per pixel..." << std::endl;

        int num_threads = 1;

        // if compiled with openmp and --parallel flag is present, enable multi-threading.
        #ifdef _OPENMP
        if (!enable_parallel) {
            // explicitly set to single-threaded if parallel is not requested.
            omp_set_num_threads(1);
        }
        // get the number of threads that will be used.
        num_threads = omp_get_max_threads();
        std::cout << "Number of threads: " << num_threads << std::endl;

        #else
                    if (enable_parallel) {
                        std::cout << "Warning: --parallel flag ignored. Program was not compiled with OpenMP." << std::endl;
                    }
        #endif

        // atomic counter for tracking progress across threads.
        std::atomic<int> scanlines_completed(0);
        int total_scanlines = height;
        int last_reported_progress = -1;

        // openmp pragma to parallelize the outer loop over scanlines.
        #ifdef _OPENMP
        #pragma omp parallel for schedule(dynamic, 10)
        #endif
        for (int y = 0; y < height; ++y) {
            // for every pixel in the current scanline.
            for (int x = 0; x < width; ++x) {
                // initialises the colour accumulator for the pixel.
                Vector3 pixel_color_vec(0, 0, 0);

                // anti-aliasing loop: cast multiple rays per pixel.
                for (int s = 0; s < SAMPLES_PER_PIXEL; ++s) {

                    // generate a random offset within the pixel for anti-aliasing.
                    float random_u = random_double();
                    float random_v = random_double();

                    // calculate the normalized (u,v) coordinate for the ray, with random jitter.
                    float px = (static_cast<float>(x) + random_u) / width;
                    float py = (static_cast<float>(y) + random_v) / height;

                    // calculate a random time for the ray for motion blur.
                    double ray_time = random_double() * scene.get_shutter_time();

                    // generate a ray for the current sample. defines the origin, direction and time.
                    Ray ray = camera.generateRay(px, py, ray_time);

                    // trace the ray and accumulate the resulting color.
                    pixel_color_vec = pixel_color_vec + ray_colour(ray, scene, world, MAX_DEPTH);
                }
                // calculate the average color from all samples for the pixel.
                Vector3 averaged_color_vec = pixel_color_vec * (1.0 / SAMPLES_PER_PIXEL);
                // apply tonemapping
                if (tonemap_mode == 1) {
                    averaged_color_vec = tonemap_reinhard(averaged_color_vec);
                } else if (tonemap_mode == 2) {
                    averaged_color_vec = tonemap_aces(averaged_color_vec);
                } else if (tonemap_mode == 3) {
                    averaged_color_vec = tonemap_filmic(averaged_color_vec);
                }
                // convert the final vector color to a pixel format (e.g., 8-bit rgb).
                Pixel final_color = final_colour_to_pixel(averaged_color_vec);
                // set the pixel color in the image buffer.
                image.setPixel(x, y, final_color);

            }
            // progress reporting logic.
            #ifdef _OPENMP
            // only thread 0 reports progress to avoid garbled output.
            if (omp_get_thread_num() == 0) {
            #endif
                // atomically increment the completed scanlines counter.
                int completed = scanlines_completed.fetch_add(1) + 1;
                // calculate the percentage of completion.
                int percent = (static_cast<long long>(completed) * 100) / total_scanlines;
                // report progress every 5% or on completion.
                if (percent > last_reported_progress && (percent % 5 == 0 || completed == total_scanlines)) {
                    last_reported_progress = percent;
                    std::stringstream ss;
                    // build the progress string.
                    ss << "\rRendering: " << percent << "% [" << completed << "/" << total_scanlines << "]";
                    // add a newline on completion.
                    if (completed == total_scanlines) ss << std::endl;
                    // print the progress string to the console.
                    std::cout << ss.str() << std::flush;
                }
            #ifdef _OPENMP
            } else {
                scanlines_completed.fetch_add(1);
            }
            #endif
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end_time - start_time;

        if (!output_path.empty()) {
            image.write(output_path);
            std::cout << "Image saved to '" << output_path << "'." << std::endl;
        }

        return elapsed.count();
    };

    // BVH testing
    if (enable_bvh_testing) {
        std::string timestamp_str = get_current_timestamp();
        std::string output_dir = "../../Output/testing/" + timestamp_str;
        std::string source_bvh_dir = "../../ASCII/BVH_tests";

        try {
            fs::create_directories(output_dir);
            std::cout << "Output directory created: " << output_dir << std::endl;

            // Copy BVH_tests folder
            if (fs::exists(source_bvh_dir)) {
                fs::copy(source_bvh_dir, output_dir + "/BVH_tests", fs::copy_options::recursive | fs::copy_options::overwrite_existing);
                std::cout << "Copied BVH_tests to output directory." << std::endl;
            } else {
                std::cerr << "Warning: BVH_tests directory not found at " << source_bvh_dir << std::endl;
            }

            // Write flags to txt file
            std::ofstream flags_file(output_dir + "/flags.txt");
            if (flags_file.is_open()) {
                flags_file << "Flags used: " << all_args << std::endl;
                flags_file.close();
            }

            // Find all scene_x.txt files
            struct TestScene {
                int x;
                std::string path;
            };
            std::vector<TestScene> test_scenes;
            std::regex scene_regex(R"(scene_?(\d+)\.txt)", std::regex::icase);
            std::smatch match;

            std::cout << "Scanning for scenes in: " << source_bvh_dir << std::endl;

            if (fs::exists(source_bvh_dir)) {
                for (const auto& entry : fs::directory_iterator(source_bvh_dir)) {
                    // Only check regular files
                    if (!entry.is_regular_file()) continue;

                    std::string filename = entry.path().filename().string();
                    std::cout << "  Checking file: " << filename;

                    if (std::regex_search(filename, match, scene_regex)) {
                        int val = std::stoi(match[1]);
                        std::cout << " [MATCHED, X=" << val << "]" << std::endl;
                        test_scenes.push_back({val, entry.path().string()});
                    }
                    // Added fallback for standard 'scene.txt'
                    else if (filename == "scene.txt") {
                        std::cout << " [MATCHED, treated as X=0]" << std::endl;
                        test_scenes.push_back({0, entry.path().string()});
                    }
                    else {
                        std::cout << " [NO MATCH]" << std::endl;
                    }
                }
            } else {
                std::cerr << "Error: Source directory does not exist, cannot scan for files." << std::endl;
            }

            if (test_scenes.empty()) {
                std::cerr << "No matching scene files found! Please check filenames in " << source_bvh_dir << std::endl;
                return 1;
            }

            // Sort by X
            std::sort(test_scenes.begin(), test_scenes.end(), [](const TestScene& a, const TestScene& b) {
                return a.x < b.x;
            });

            std::ofstream bvh_out(output_dir + "/bvh_test.txt");
            std::ofstream no_bvh_out(output_dir + "/no_bvh_test.txt");

            if (!bvh_out.is_open() || !no_bvh_out.is_open()) {
                std::cerr << "Error opening output result files." << std::endl;
                return 1;
            }

            // Loop through each scene and run tests
            for (const auto& scene : test_scenes) {
                std::cout << "\n--- Testing Scene X=" << scene.x << " ---" << std::endl;

                // Construct output paths for images
                std::string bvh_img_path = output_dir + "/bvh_" + std::to_string(scene.x) + ".ppm";
                std::string no_bvh_img_path = output_dir + "/no_bvh_" + std::to_string(scene.x) + ".ppm";

                // Run 5 times with BVH
                double total_time_bvh = 0;
                for (int i = 0; i < 3; ++i) {
                    std::cout << "Run " << (i+1) << "/3 [BVH ON]" << std::endl;
                    std::string current_output = (i == 0) ? bvh_img_path : "";
                    total_time_bvh += render_scene_func(scene.path, true, current_output);
                }
                double avg_time_bvh = total_time_bvh / 3.0;
                bvh_out << avg_time_bvh << " " << scene.x << std::endl;

                // Run 5 times without BVH
                double total_time_no_bvh = 0;
                for (int i = 0; i < 3; ++i) {
                    std::cout << "Run " << (i+1) << "/3 [BVH OFF]" << std::endl;
                    // Only write the image on the first run
                    std::string current_output = (i == 0) ? no_bvh_img_path : "";
                    total_time_no_bvh += render_scene_func(scene.path, false, current_output);
                }
                double avg_time_no_bvh = total_time_no_bvh / 3.0;
                no_bvh_out << avg_time_no_bvh << " " << scene.x << std::endl;
            }

            std::cout << "\nBVH Testing Complete. Results saved to " << output_dir << std::endl;

        } catch (const std::exception& e) {
            std::cerr << "Error during BVH testing: " << e.what() << std::endl;
            return 1;
        }

        return 0; // Exit after testing
    }

    // standard mode

    std::string timestamp_str;
    std::string output_dir;
    // if timing is enabled, set up a directory for test outputs.
    if (enable_timing) {
        // get a timestamp for the test run.
        timestamp_str = get_current_timestamp();
        // construct the output directory path.
        output_dir = "../../Output/testing/" + timestamp_str;
        try {
            // create the output directory and any necessary parent directories.
            fs::create_directories(output_dir);
            std::cout << "Output directory created: " << output_dir << std::endl;

            std::string source_scene = "../../ASCII/scene.txt";
            std::string dest_scene = output_dir + "/scene.txt";

            // copy the scene file to the test output directory for record-keeping.
            fs::copy(source_scene, dest_scene, fs::copy_options::overwrite_existing);
            std::cout << "Saved copy of scene.txt to output folder." << std::endl;

        } catch (const std::exception& e) {
            std::cerr << "Error creating output directory: " << e.what() << std::endl;
            return 1;
        }
    }

    // structure to store statistics for each render run.
    struct RunStats {
        double duration;
        std::string output_path;
    };
    std::vector<RunStats> timing_results;

    // main render loop, runs multiple times if timing is enabled.
    try {
        const std::string scene_file = "../../ASCII/scene.txt";

        for (int i = 0; i < run_count; ++i) {
            if (enable_timing) {
                std::cout << "\n--- Starting Run " << (i + 1) << " of " << run_count << " ---" << std::endl;
            }

            std::string output_file;
            if (enable_timing) {
                // construct a unique filename for each timed run.
                output_file = output_dir + "/output_" + timestamp_str + "_" + std::to_string(i + 1) + ".ppm";
            } else {
                output_file = "../../Output/scene_test.ppm";
            }

            // Call the encapsulated render function
            double elapsed = render_scene_func(scene_file, use_bvh, output_file);

            if (enable_timing) {
                std::cout << "Run " << (i + 1) << " completed in " << elapsed << " seconds." << std::endl;
                // store the results of the timed run.
                timing_results.push_back({elapsed, output_file});
            } else {
                std::cout << "Render complete! Image saved to '" << output_file << "'." << std::endl;
            }
        }

        // after all runs, if timing was enabled, write a log file.
        if (enable_timing) {
            std::string log_path = output_dir + "/timing_log.txt";
            std::ofstream log_file(log_path);
            if (log_file.is_open()) {
                // log the command-line arguments used for the test.
                log_file << "args: [" << all_args << "]\n";
                // log the duration and output path for each run.
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
