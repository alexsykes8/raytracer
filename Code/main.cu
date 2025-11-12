#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

#include <cuda_runtime.h>

#include "device_types.cuh"
#include "scene_parser.h"
#include "kernel.cuh"

#include "Image.h"

#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            std::cerr << "CUDA Error at " << __FILE__ << ":" << __LINE__ \
                      << ": " << cudaGetErrorString(err) << std::endl; \
            exit(EXIT_FAILURE); \
        } \
    } while (0)



int main(int argc, char* argv[]) {
    double exposure = 1.0;
    bool enable_shadows = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--exposure") {
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
                std::cerr << "Error: --exposure flag requires a value." << std::endl;
                return 1;
            }
        } else if (arg == "--shadows") {
            enable_shadows = true;
            std::cout << "Shadows enabled" << std::endl;
        }
    }

    try {
        std::cout << "Loading scene..." << std::endl;
        const std::string scene_file = "../../ASCII/scene.txt";

        SceneParser parser(scene_file, exposure, enable_shadows);

        const int width = parser.h_width;
        const int height = parser.h_height;

        if (width == 0 || height == 0) {
            throw std::runtime_error("Scene file error: Invalid resolution.");
        }

        std::cout << "Preparing GPU data..." << std::endl;


        unsigned char* d_image_data;
        size_t image_size = width * height * 3 * sizeof(unsigned char);
        CUDA_CHECK(cudaMalloc(&d_image_data, image_size));

        Sphere* d_spheres = nullptr;
        Cube* d_cubes = nullptr;
        Plane* d_planes = nullptr;
        PointLight* d_lights = nullptr;

        size_t spheres_size = parser.h_spheres.size() * sizeof(Sphere);
        if (spheres_size > 0) CUDA_CHECK(cudaMalloc(&d_spheres, spheres_size));

        size_t cubes_size = parser.h_cubes.size() * sizeof(Cube);
        if (cubes_size > 0) CUDA_CHECK(cudaMalloc(&d_cubes, cubes_size));

        size_t planes_size = parser.h_planes.size() * sizeof(Plane);
        if (planes_size > 0) CUDA_CHECK(cudaMalloc(&d_planes, planes_size));

        size_t lights_size = parser.h_lights.size() * sizeof(PointLight);
        if (lights_size > 0) CUDA_CHECK(cudaMalloc(&d_lights, lights_size));

        if (spheres_size > 0)
            CUDA_CHECK(cudaMemcpy(d_spheres, parser.h_spheres.data(), spheres_size, cudaMemcpyHostToDevice));
        if (cubes_size > 0)
            CUDA_CHECK(cudaMemcpy(d_cubes, parser.h_cubes.data(), cubes_size, cudaMemcpyHostToDevice));
        if (planes_size > 0)
            CUDA_CHECK(cudaMemcpy(d_planes, parser.h_planes.data(), planes_size, cudaMemcpyHostToDevice));
        if (lights_size > 0)
            CUDA_CHECK(cudaMemcpy(d_lights, parser.h_lights.data(), lights_size, cudaMemcpyHostToDevice));

        std::cout << "Rendering scene (" << width << "x" << height << ")..." << std::endl;

        dim3 threads_per_block(16, 16); // 16x16 = 256 threads per block
        dim3 num_blocks(
            (width + threads_per_block.x - 1) / threads_per_block.x,
            (height + threads_per_block.y - 1) / threads_per_block.y
        );

        render_kernel<<<num_blocks, threads_per_block>>>(
            d_image_data,
            width, height,
            parser.h_camera,
            parser.h_exposure,
            parser.h_enable_shadows,
            d_lights, parser.h_lights.size(),
            d_spheres, parser.h_spheres.size(),
            d_cubes, parser.h_cubes.size(),
            d_planes, parser.h_planes.size()
        );

        CUDA_CHECK(cudaGetLastError());

        CUDA_CHECK(cudaDeviceSynchronize());
        std::cout << "Render complete!" << std::endl;


        std::vector<unsigned char> h_image_data(width * height * 3);

        CUDA_CHECK(cudaMemcpy(
            h_image_data.data(), // Destination (CPU)
            d_image_data,        // Source (GPU)
            image_size,          // Size
            cudaMemcpyDeviceToHost
        ));


        Image image(width, height);


        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * 3;
                image.setPixel(x, y, {
                    h_image_data[idx + 0], // R
                    h_image_data[idx + 1], // G
                    h_image_data[idx + 2]  // B
                });
            }
        }

        const std::string output_file = "../../Output/scene_test.ppm";
        image.write(output_file);
        std::cout << "Render complete! Image saved to '" << output_file << "'." << std::endl;

        CUDA_CHECK(cudaFree(d_image_data));
        if (d_spheres) CUDA_CHECK(cudaFree(d_spheres));
        if (d_cubes)   CUDA_CHECK(cudaFree(d_cubes));
        if (d_planes)  CUDA_CHECK(cudaFree(d_planes));
        if (d_lights)  CUDA_CHECK(cudaFree(d_lights));

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

