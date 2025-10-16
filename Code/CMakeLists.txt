cmake_minimum_required(VERSION 4.0)
project(B216602)

set(CMAKE_CXX_STANDARD 20)

add_executable(B216602 main.cpp Image.cpp
        camera.cpp
        camera.h
        vector3.cpp
        vector3.h
        ray.h
        test_rays_image.cpp
        test_rays_image.h)
