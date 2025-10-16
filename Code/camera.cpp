//
// Created by alex on 16/10/2025.
//

#include "camera.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include "vector3.h"

// Helper to read 3 doubles into a Vector3 from a string stream
void read_vector(std::stringstream& ss, Vector3& vec) {
    // the following if fails if the stream hits the end of the file or finds text instead of number before all 3 components are read
    if (!(ss >> vec.x >> vec.y >> vec.z)) {
        throw std::runtime_error("Error reading vector components.");
    }
}


Camera::Camera(const std::string& scene_filepath)
    // initialise camera properties to 0
    : m_focal_length(0.0), m_sensor_width(0.0), m_sensor_height(0.0),
      m_resolution_x(0), m_resolution_y(0) {

    readSceneFile(scene_filepath);

    // After reading data, compute the orthonormal basis for ray generation.
    computeCameraBasis();
}

void Camera::readSceneFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open scene file: " + filepath);
    }

    std::string line;
    // initialise the search for the block of scene.txt referring to camera data
    bool in_camera_block = false;

    // Use a simplified parsing loop that only looks for the CAMERA block and its fields.
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "CAMERA") {
            in_camera_block = true;
            continue;
        } else if (token == "END_CAMERA") {
            in_camera_block = false;
            // Break early once camera data is found and processed
            break;
        }

        if (in_camera_block) {
            if (token == "location") {
                read_vector(ss, m_location);
            } else if (token == "gaze_direction") {
                read_vector(ss, m_gaze_direction_hint);
            } else if (token == "up_vector") {
                read_vector(ss, m_up_vector_hint);
            } else if (token == "focal_length") {
                ss >> m_focal_length;
            } else if (token == "sensor_size") {
                ss >> m_sensor_width >> m_sensor_height;
            } else if (token == "resolution") {
                ss >> m_resolution_x >> m_resolution_y;
            }
            // Ignore rotation_euler_radians/degrees for this implementation,
            // relying on the more robust gaze/up vectors.
            // euler info just for testing and visualisation.
        }
    }

    // Basic validation after reading
    if (m_resolution_x == 0 || m_focal_length == 0.0) {
        throw std::runtime_error("Missing or invalid camera data in scene file.");
    }
}

// Helper to compute the Camera's Orthonormal Basis (The LookAt transformation)

void Camera::computeCameraBasis() {
    // Calculate W (The Gaze/Forward vector)
    // The gaze direction is the forward vector, W. Normalize it to work with raytracing equations.
    m_camera_w = m_gaze_direction_hint.normalize();

    // Calculate U (The Right vector)
    // U is perpendicular to both the UP hint and the Gaze direction.
    // U = W x UP_HINT
    // Since we want a right-handed system where U is right, V is up, and W is forward:
    // use the UP_HINT x W cross product, then normalize.
    m_camera_u = m_up_vector_hint.cross(m_camera_w).normalize();

    // Calculate V (The True Up vector)
    // V must be perpendicular to both U and W to form a perfect orthonormal basis.
    // V = U x W
    m_camera_v = m_camera_u.cross(m_camera_w);
}

// Convert pixel coordinates to a ray in world coordinates.

Ray Camera::generateRay(float px, float py) const {

    // --- Map pixel coordinates (px, py) to the image plane ---

    // px and py are the normalized coordinates (0 to 1).
    // Center the coordinates so (0.5, 0.5) is the center of the image.
    // The image plane's size is determined by the sensor width/height.

    // Scale the normalized pixel coordinates (0 to 1) to the actual sensor/view plane dimensions.
    // Also, map (0,0) from the top-left of the pixel grid (raster space) to the center of the pixel,
    // and adjust the y-axis (raster Y is down, world Y is usually up).

    // Calculate fractional pixel size relative to the sensor size
    double pixel_width_m = m_sensor_width / m_resolution_x;
    double pixel_height_m = m_sensor_height / m_resolution_y;

    // Calculate the coordinate on the sensor plane relative to the center of the sensor (in camera's local U/V space)

    // Center the pixel at 0.0, and scale by sensor width
    double u_coord = (px - 0.5) * m_sensor_width;

    // Center the pixel at 0.0, scale by sensor height, and flip the Y-axis (since pixel 0 is top)
    double v_coord = (0.5 - py) * m_sensor_height;

    // --- Calculate the Ray Direction in World Space ---

    // The center of the image plane is defined by the focal length along the W (gaze) vector.
    // The point on the image plane (P) is found by moving from the center of the sensor by
    // u_coord along U and v_coord along V.

    // The image plane point in World Coordinates is P_world = (FocalLength * W) + (u_coord * U) + (v_coord * V)

    Vector3 image_plane_point =
        (m_focal_length * m_camera_w) +
        (u_coord * m_camera_u) +
        (v_coord * m_camera_v);

    // The ray direction is the vector from the camera's origin (m_location) to the point on the image plane.
    // Since the ray direction is calculated relative to the camera origin (P_world - m_location),
    // and since m_location is added to all components to get P_world, the ray direction is the image_plane_point vector itself.
    // Ray_Direction = (P_world - Origin) / |P_world - Origin|

    // Since the camera is at the origin of its local coordinate system, the vector from the origin to
    // the point on the image plane *is* the direction vector.
    Vector3 ray_direction = image_plane_point.normalize();

    // --- Construct the Ray ---
    Ray r;
    r.origin = m_location;
    r.direction = ray_direction; // Already normalized

    return r;
}
