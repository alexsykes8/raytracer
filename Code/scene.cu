#include "scene_parser.h"
#include "camera.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <cstring> // For memcpy

static void read_vector(std::stringstream& ss, Vector3& vec) {
    if (!(ss >> vec.x >> vec.y >> vec.z)) {
        throw std::runtime_error("Error reading vector components.");
    }
}

SceneParser::~SceneParser() = default;

SceneParser::SceneParser(const std::string& scene_filepath, double exposure, bool enable_shadows)
    : h_exposure(exposure), h_enable_shadows(enable_shadows)
{

    parseSceneFile(scene_filepath);

    if (!m_host_camera) {
        throw std::runtime_error("Scene file error: No camera data found.");
    }

    // Convert the complex host-side Camera object into simple
    //    CUDACamera struct for the GPU.

    h_camera.origin = m_host_camera->getLocation();

    // Get the pre-calculated basis vectors from the host camera
    Vector3 u = m_host_camera->getU(); // "Right" vector
    Vector3 v = m_host_camera->getV(); // "True Up" vector
    Vector3 w = m_host_camera->getW(); // "Gaze" vector

    double sensor_w = m_host_camera->getSensorWidth();
    double sensor_h = m_host_camera->getSensorHeight();
    double focal_len = m_host_camera->getFocalLength();

    double viewport_height = (sensor_h / focal_len);
    double viewport_width = (sensor_w / focal_len);


    h_camera.horizontal = viewport_width * u;
    h_camera.vertical = viewport_height * v;

    Vector3 viewport_center = h_camera.origin + w;

    h_camera.lower_left_corner = viewport_center - 0.5 * h_camera.horizontal - 0.5 * h_camera.vertical;

    h_width = m_host_camera->getResolutionX();
    h_height = m_host_camera->getResolutionY();


    std::cout << "SceneParser: Loaded " << h_spheres.size() << " spheres, "
              << h_cubes.size() << " cubes, "
              << h_planes.size() << " planes, "
              << h_lights.size() << " lights." << std::endl;
}


void SceneParser::parseSceneFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open scene file: " + filepath);
    }

    std::string line;
    std::string current_block_type = "NONE";

    Vector3 cam_location, cam_gaze, cam_up;
    double cam_focal = 0.0, cam_sensor_w = 0.0, cam_sensor_h = 0.0;
    int cam_res_x = 0, cam_res_y = 0;

    Vector3 translation, rotation, scale_vec;
    std::vector<Vector3> temp_corners;
    Vector3 light_pos, light_intensity;

    Material temp_mat;

        while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;
        ss >> token;

        if (token.empty() || token[0] == '#') continue;

        if (token == "CAMERA") { current_block_type = "CAMERA"; continue; }
        if (token == "POINT_LIGHT") {
            current_block_type = "POINT_LIGHT";
            light_pos = Vector3(0,0,0);
            light_intensity = Vector3(1,1,1);
            continue;
        }
        if (token == "SPHERE") {
            current_block_type = "SPHERE";
            translation = Vector3(0, 0, 0);
            rotation = Vector3(0, 0, 0);
            scale_vec = Vector3(1, 1, 1);
            temp_mat = Material();
            continue;
        }
        if (token == "CUBE") {
            current_block_type = "CUBE";
            translation = Vector3(0, 0, 0);
            rotation = Vector3(0, 0, 0);
            scale_vec = Vector3(1, 1, 1);
            temp_mat = Material();
            continue;
        }
        if (token == "PLANE") {
            current_block_type = "PLANE";
            temp_corners.clear();
            temp_mat = Material();
            continue;
        }

        if (token == "END_CAMERA") {
            m_host_camera = std::make_unique<Camera>(
                cam_location, cam_gaze, cam_up,
                cam_focal, cam_sensor_w, cam_sensor_h,
                cam_res_x, cam_res_y
            );
            current_block_type = "NONE";
            continue;
        }

        if (token == "END_POINT_LIGHT") {
            h_lights.push_back(PointLight{light_pos, light_intensity});
            current_block_type = "None";
            continue;
        }

        if (token == "END_SPHERE") {
            Matrix4x4 mat_s = Matrix4x4::createScale(scale_vec);
            Matrix4x4 mat_rx = Matrix4x4::createRotationX(rotation.x);
            Matrix4x4 mat_ry = Matrix4x4::createRotationY(rotation.y);
            Matrix4x4 mat_rz = Matrix4x4::createRotationZ(rotation.z);
            Matrix4x4 mat_t = Matrix4x4::createTranslation(translation);

            Matrix4x4 transform = mat_t * mat_rz * mat_ry * mat_rx * mat_s;
            Matrix4x4 inv_transform = transform.inverse();
            Matrix4x4 inv_transpose = inv_transform.transpose();

            Sphere s;
            s.mat = temp_mat;
            s.transform = transform;
            s.inverse_transform = inv_transform;
            s.inverse_transpose = inv_transpose;

            h_spheres.push_back(s);
            current_block_type = "NONE";
            continue;
        }

        if (token == "END_CUBE") {
            Matrix4x4 mat_s = Matrix4x4::createScale(scale_vec);
            Matrix4x4 mat_rx = Matrix4x4::createRotationX(rotation.x);
            Matrix4x4 mat_ry = Matrix4x4::createRotationY(rotation.y);
            Matrix4x4 mat_rz = Matrix4x4::createRotationZ(rotation.z);
            Matrix4x4 mat_t = Matrix4x4::createTranslation(translation);

            Matrix4x4 transform = mat_t * mat_rz * mat_ry * mat_rx * mat_s;
            Matrix4x4 inv_transform = transform.inverse();
            Matrix4x4 inv_transpose = inv_transform.transpose();

            Cube c;
            c.mat = temp_mat;
            c.transform = transform;
            c.inverse_transform = inv_transform;
            c.inverse_transpose = inv_transpose;

            h_cubes.push_back(c);
            current_block_type = "NONE";
            continue;
        }

        if (token == "END_PLANE") {
            if (temp_corners.size() == 4) {
                Plane p;
                p.p0 = temp_corners[0];
                p.p1 = temp_corners[1];
                p.p2 = temp_corners[2];
                p.p3 = temp_corners[3];
                p.mat = temp_mat;

                Vector3 v1 = p.p1 - p.p0;
                Vector3 v2 = p.p2 - p.p0;
                p.normal = v1.cross(v2).normalize();

                h_planes.push_back(p);
            } else {
                std::cerr << "Warning: Plane block ended with " << temp_corners.size() << " corners, expected 4." << std::endl;
            }
            current_block_type = "NONE";
            continue;
        }

        if (current_block_type == "CAMERA") {
            if (token == "location") { read_vector(ss, cam_location); }
            else if (token == "gaze_direction") { read_vector(ss, cam_gaze); }
            else if (token == "up_vector") { read_vector(ss, cam_up); }
            else if (token == "focal_length") { ss >> cam_focal; }
            else if (token == "sensor_size") { ss >> cam_sensor_w >> cam_sensor_h; }
            else if (token == "resolution") { ss >> cam_res_x >> cam_res_y; }
        }
        else if (current_block_type == "POINT_LIGHT") {
            if (token == "location") {read_vector(ss, light_pos); }
            else if (token == "intensity") { read_vector(ss, light_intensity); }
        }
        else if (current_block_type == "SPHERE" || current_block_type == "CUBE" || current_block_type == "PLANE") {

            if (token == "translation") { read_vector(ss, translation); }
            else if (token == "rotation_euler_radians") { read_vector(ss, rotation); }
            else if (token == "scale") { read_vector(ss, scale_vec); }
            else if (token == "corner") {
                Vector3 corner;
                read_vector(ss, corner);
                temp_corners.push_back(corner);
            }
            else if (token == "ambient") { read_vector(ss, temp_mat.ambient); }
            else if (token == "diffuse") { read_vector(ss, temp_mat.diffuse); }
            else if (token == "specular") { read_vector(ss, temp_mat.specular); }
            else if (token == "shininess") { ss >> temp_mat.shininess; }
            else if (token == "reflectivity") { ss >> temp_mat.reflectivity; }
            else if (token == "transparency") { ss >> temp_mat.transparency; }
            else if (token == "refractive_index") { ss >> temp_mat.refractive_index; }
        }
    }
}