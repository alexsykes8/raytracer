#include "scene.h"
#include "shapes/sphere.h"
#include "shapes/plane.h"
#include "shapes/cube.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <memory>
#include "matrix4x4.h"
#include <vector>
#include "acceleration/bvh.h"
#include "material.h"
#include "Image.h"


// Helper that reads three doubles from a stream and store into a Vector3 object.
static void read_vector(std::stringstream& ss, Vector3& vec) {
    if (!(ss >> vec.x >> vec.y >> vec.z)) {
        throw std::runtime_error("Error reading vector components.");
    }
}

Scene::Scene(const std::string& scene_filepath, bool build_bvh, double exposure, bool enable_shadows, int glossy_samples)
            : m_exposure(exposure) , m_shadows_enabled(enable_shadows), m_glossy_samples(glossy_samples) {
    // m_camera is initially null
    parseSceneFile(scene_filepath);

    if (!m_camera) {
        throw std::runtime_error("Scene file error: No camera data found.");
    }

    if (build_bvh) {
        // prepare a bounding volume hierarchy (BVH)
        if (!m_world.objects.empty()) {
            std::cout << "Building BVH..." << std::endl;

            // creates a BVHNode object on the heap and wraps it in a pointer that points to the root node of the BVH tree.
            auto bvh_root = std::make_shared<BVHNode>(m_world);

            // removes the individual pointers to the object in the world nad replaces them with a pointer to the BVH tree.
            m_world.objects.clear();
            m_world.add(bvh_root);

            std::cout << "BVH build complete." << std::endl;
        } else {
            // there are no objects in the world to build a BVH from.
            std::cout << "Scene is empty, skipping BVH build." << std::endl;
        }
    } else {
        std::cout << "BVH build skipped." << std::endl;
    }
}

void Scene::parseSceneFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open scene file: " + filepath);
    }

    std::string line;
    std::string current_block_type = "NONE";

    // Temporary storage for camera parameters
    Vector3 cam_location, cam_gaze, cam_up;
    double cam_focal = 0.0, cam_sensor_w = 0.0, cam_sensor_h = 0.0;
    int cam_res_x = 0, cam_res_y = 0;
    double cam_f_stop = 99999.0;
    double cam_focal_distance = 10.0;

    // Temporary storage for shape values.
    Vector3 translation, rotation, scale_vec;

    // Temporary storage for the corners of the plane.
    std::vector<Vector3> temp_corners;

    // Temporary storage for the lights.
    Vector3 light_pos, light_intensity;
    double light_radius = 0.0;

    // Temporary storage for material properties.
    Material temp_mat;


    while (std::getline(file, line)) {
        std::cout << "Processing line: " << line << std::endl;
        std::stringstream ss(line);
        std::string token;
        // tokenize each line of the file
        ss >> token;

        // Switch between the different blocks
        if (token.empty() || token[0] == '#') continue; // Skip empty/comment lines
        if (token == "CAMERA") { current_block_type = "CAMERA"; continue; }

        if (token == "POINT_LIGHT") {
            current_block_type = "POINT_LIGHT";
            light_pos = Vector3(0,0,0);
            light_intensity = Vector3(1,1,1);
            light_radius = 0.0;
            continue;
        }

        if (token == "SPHERE") {
            current_block_type = "SPHERE";
            translation = Vector3(0, 0, 0);
            rotation = Vector3(0, 0, 0);
            scale_vec = Vector3(1, 1, 1); // Default scale is (1,1,1)
            temp_mat = Material();
            continue;
        }
        if (token == "CUBE") {
            current_block_type = "CUBE";
            translation = Vector3(0, 0, 0);
            rotation = Vector3(0, 0, 0);
            scale_vec = Vector3(1, 1, 1); // Default scale is (1,1,1)
            temp_mat = Material();
            continue;
        }

        if (token == "PLANE") {
            current_block_type = "PLANE";
            // discards corner data from previous planes. The corners are set lower down at the else if (current_block_type == "PLANE")
            temp_corners.clear();
            temp_mat = Material();
            continue;
        }


        // End Block Logic
        if (token == "END_CAMERA") {
            // All camera data is read, create the Camera object
            m_camera = std::make_unique<Camera>(
                cam_location, cam_gaze, cam_up,
                cam_focal, cam_sensor_w, cam_sensor_h,
                cam_res_x, cam_res_y,
                cam_f_stop, cam_focal_distance
            );
            current_block_type = "NONE";
            continue;
        }

        if (token == "END_POINT_LIGHT") {
            m_lights.push_back(PointLight(light_pos, light_intensity, light_radius));
            current_block_type = "None";
            continue;
        }

        if (token == "END_SPHERE") {
            if (!temp_mat.texture_filename.empty()) {
                std::string texture_path = "../" + temp_mat.texture_filename;
                try {
                    temp_mat.texture = std::make_shared<Image>(texture_path);
                    std::cout << "  Successfully loaded texture: " << texture_path << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "  Error loading texture: " << e.what() << std::endl;
                    temp_mat.texture = nullptr;
                }
            }
            // Build Transformation Matrices
            Matrix4x4 mat_s = Matrix4x4::createScale(scale_vec);
            Matrix4x4 mat_rx = Matrix4x4::createRotationX(rotation.x);
            Matrix4x4 mat_ry = Matrix4x4::createRotationY(rotation.y);
            Matrix4x4 mat_rz = Matrix4x4::createRotationZ(rotation.z);
            Matrix4x4 mat_t = Matrix4x4::createTranslation(translation);

            // Combine transforms: T * Rz * Ry * Rx * S
            // Produces the final object-to-world transformation matrix for the sphere.
            Matrix4x4 transform = mat_t * mat_rz * mat_ry * mat_rx * mat_s;
            // Calculates the inverse of the transform matrix to convert world space back to local object space.
            Matrix4x4 inv_transform = transform.inverse();

            // Add the completed shape
            m_world.add(std::make_shared<Sphere>(transform, inv_transform, temp_mat));
            current_block_type = "NONE";
            continue;
        }
        if (token == "END_CUBE") {
            if (!temp_mat.texture_filename.empty()) {
                std::string texture_path = "../" + temp_mat.texture_filename;
                try {
                    temp_mat.texture = std::make_shared<Image>(texture_path);
                    std::cout << "  Successfully loaded texture: " << texture_path << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "  Error loading texture: " << e.what() << std::endl;
                    temp_mat.texture = nullptr;
                }
            }
            // Build transforms for Cube
            Matrix4x4 mat_s = Matrix4x4::createScale(scale_vec);
            Matrix4x4 mat_rx = Matrix4x4::createRotationX(rotation.x);
            Matrix4x4 mat_ry = Matrix4x4::createRotationY(rotation.y);
            Matrix4x4 mat_rz = Matrix4x4::createRotationZ(rotation.z);
            Matrix4x4 mat_t = Matrix4x4::createTranslation(translation);

            // Combine transforms: T * Rz * Ry * Rx * S
            // Produces the final object-to-world transformation matrix for the cube.
            Matrix4x4 transform = mat_t * mat_rz * mat_ry * mat_rx * mat_s;
            // Calculates the inverse of the transform matrix to convert world space back to local object space.
            Matrix4x4 inv_transform = transform.inverse();

            // Add the object to the world.
            m_world.add(std::make_shared<Cube>(transform, inv_transform, temp_mat));

            current_block_type = "NONE";
            continue;
        }

        if (token == "END_PLANE") {
            if (!temp_mat.texture_filename.empty()) {
                std::string texture_path = "../" + temp_mat.texture_filename;
                try {
                    temp_mat.texture = std::make_shared<Image>(texture_path);
                    std::cout << "  Successfully loaded texture: " << texture_path << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "  Warning: Could not load texture '" << texture_path << "'. " << e.what() << std::endl;
                    temp_mat.texture = nullptr;
                }
            }
            if (temp_corners.size() == 4) {
                // Add the plane object to the world.
                m_world.add(std::make_shared<Plane>(
                    temp_corners[0], temp_corners[1], temp_corners[2], temp_corners[3], temp_mat));
            } else {
                // Error if the plane does not have corners.
                std::cerr << "Warning: Plane block ended with " << temp_corners.size() << " corners, expected 4." << std::endl;
            }
            current_block_type = "NONE";
            continue;
        }


        // Block Data Parsing
        // Takes the tokens and assigns them to variables.
        if (current_block_type == "CAMERA") {
            if (token == "location") { read_vector(ss, cam_location); }
            else if (token == "gaze_direction") { read_vector(ss, cam_gaze); }
            else if (token == "up_vector") { read_vector(ss, cam_up); }
            else if (token == "focal_length") { ss >> cam_focal; }
            else if (token == "sensor_size") { ss >> cam_sensor_w >> cam_sensor_h; }
            else if (token == "resolution") { ss >> cam_res_x >> cam_res_y; }
            else if (token == "f_stop") { ss >> cam_f_stop; }
            else if (token == "focal_distance") { ss >> cam_focal_distance; }
        }
        else if (current_block_type == "POINT_LIGHT") {
            if (token == "location") {read_vector(ss, light_pos); }
            else if (token == "intensity") {
                read_vector(ss, light_intensity);
            }
            else if (token == "radius") {
                ss >> light_radius;
            }
        }
        else if (current_block_type == "SPHERE") {
            if (token == "translation") { read_vector(ss, translation); }
            else if (token == "rotation_euler_radians") { read_vector(ss, rotation); }
            else if (token == "scale") { read_vector(ss, scale_vec); }
            else if (token == "ambient") { read_vector(ss, temp_mat.ambient); }
            else if (token == "diffuse") { read_vector(ss, temp_mat.diffuse); }
            else if (token == "specular") { read_vector(ss, temp_mat.specular); }
            else if (token == "shininess") { ss >> temp_mat.shininess; }
            else if (token == "reflectivity") { ss >> temp_mat.reflectivity; }
            else if (token == "transparency") { ss >> temp_mat.transparency; }
            else if (token == "refractive_index") { ss >> temp_mat.refractive_index; }
            else if (token == "texture_file") {
                ss >> temp_mat.texture_filename;
                std::cout << "  Found texture file token: " << temp_mat.texture_filename << std::endl;
            }
        }
        else if (current_block_type == "CUBE") {
            if (token == "translation") { read_vector(ss, translation); }
            else if (token == "rotation_euler_radians") { read_vector(ss, rotation); }
            else if (token == "scale") { read_vector(ss, scale_vec); }
            else if (token == "ambient") { read_vector(ss, temp_mat.ambient); }
            else if (token == "diffuse") { read_vector(ss, temp_mat.diffuse); }
            else if (token == "specular") { read_vector(ss, temp_mat.specular); }
            else if (token == "shininess") { ss >> temp_mat.shininess; }
            else if (token == "reflectivity") { ss >> temp_mat.reflectivity; }
            else if (token == "transparency") { ss >> temp_mat.transparency; }
            else if (token == "refractive_index") { ss >> temp_mat.refractive_index; }
            else if (token == "texture_file") {
                ss >> temp_mat.texture_filename;
                std::cout << "  Found texture file token: " << temp_mat.texture_filename << std::endl;
            }
        }
        else if (current_block_type == "PLANE") {
            if (token == "corner") {
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
            else if (token == "texture_file") {
                ss >> temp_mat.texture_filename;
                std::cout << "  Found texture file token: " << temp_mat.texture_filename << std::endl;
            }
        }
    }
}