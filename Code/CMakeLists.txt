cmake_minimum_required(VERSION 3.20)
project(B216602)

set(CMAKE_CXX_STANDARD 20)

add_executable(B216602 main.cpp utilities/Image.cpp
        environment/camera.cpp
        environment/camera.h
        utilities/vector3.h
        utilities/ray.h
        shapes/sphere.cpp
        shapes/sphere.h
        shapes/hittable_list.cpp
        shapes/hittable_list.h
        utilities/scene.cpp
        utilities/scene.h
        utilities/matrix4x4.h
        utilities/matrix4x4.cpp
        shapes/cube.cpp
        shapes/cube.h
        shapes/plane.cpp
        shapes/plane.h
        acceleration/aabb.cpp
        acceleration/aabb.h
        acceleration/bvh.cpp
        acceleration/bvh.h
        environment/light.h
        shapes/material.h
        utilities/shading.h
        utilities/tracer.h
        environment/HDRImage.cpp
        environment/HDRImage.h
        shapes/complex_cube.cpp
        shapes/complex_cube.h
        shapes/complex_sphere.cpp
        shapes/complex_sphere.h
        utilities/random_utils.h
        shapes/transformed_shape.h
        config.cpp
        config.h
        shapes/complex_plane.cpp
        shapes/complex_plane.h
)

find_package(OpenMP QUIET)
if(OpenMP_FOUND)
    message(STATUS "Found OpenMP, enabling parallel build.")
    target_compile_options(B216602 PRIVATE -fopenmp)
    target_link_libraries(B216602 PRIVATE -fopenmp)
else()
message(STATUS "OpenMP not found. Building without parallel support.")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()