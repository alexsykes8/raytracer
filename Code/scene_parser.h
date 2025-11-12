#pragma once

#include <string>
#include <vector>
#include <memory>
#include "device_types.cuh"


class Camera; 


class SceneParser {
public:

    SceneParser(const std::string& scene_filepath, double exposure, bool enable_shadows);

    ~SceneParser();

    std::vector<Sphere>     h_spheres;
    std::vector<Plane>      h_planes;
    std::vector<Cube>       h_cubes;
    std::vector<PointLight> h_lights;

    CUDACamera h_camera;

    int    h_width = 0;
    int    h_height = 0;
    double h_exposure = 1.0;
    bool   h_enable_shadows = false;

private:
    void parseSceneFile(const std::string& filepath);

    std::unique_ptr<Camera> m_host_camera; 
};