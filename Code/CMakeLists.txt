cmake_minimum_required(VERSION 3.18)

project(B216602 LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CUDA_STANDARD 17)

add_executable(B216602
        main.cu
        scene.cu
        camera.cu
        matrix4x4.cu
        Image.cu
)

target_include_directories(B216602 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


set_target_properties(B216602 PROPERTIES
        CUDA_ARCHITECTURES "70;75;80;86"
)

message(STATUS "Executable will be built at: ${CMAKE_BINARY_DIR}/B216602")