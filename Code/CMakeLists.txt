cmake_minimum_required(VERSION 4.0)
project(B216602)

set(CMAKE_CXX_STANDARD 20)

add_executable(B216602 main.cpp Image.cpp
        camera.cpp
        camera.h
        vector3.h
        ray.h
        test_rays_image.cpp
        test_rays_image.h
        shapes/sphere.cpp
        shapes/sphere.h
        shapes/hittable_list.cpp
        shapes/hittable_list.h
        scene.cpp
        scene.h
        matrix4x4.h
        matrix4x4.cpp
        shapes/cube.cpp
        shapes/cube.h
        shapes/plane.cpp
        shapes/plane.h
        acceleration/aabb.cpp
        acceleration/aabb.h
        acceleration/bvh.cpp
        acceleration/bvh.h
        light.h
        material.h
        shading.h
        tracer.h
        )

find_package(OpenMP QUIET)
if(OpenMP_FOUND)
    message(STATUS "Found OpenMP, enabling parallel build.")
    target_compile_options(B216602 PRIVATE -fopenmp)
    target_link_libraries(B216602 PRIVATE -fopenmp)
else()
message(STATUS "OpenMP not found. Building without parallel support.")
endif()