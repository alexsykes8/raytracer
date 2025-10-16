//
// Created by alex on 16/10/2025.
//

#ifndef B216602_CAMERA_H
#define B216602_CAMERA_H


#include <string>
#include <stdexcept>
#include "vector3.h" // Assuming Vector3.h contains the Vector3 class and Ray struct
#include "ray.h"     // Included via Vector3.h in the provided Vector3.h file.

/**
 * @brief Implements the pin-hole camera model and generates viewing rays.
 * This class handles reading camera parameters and the crucial coordinate transformations.
 */
class Camera {
public:
    // Constructor: Reads the scene file and stores camera information.
    explicit Camera(const std::string& scene_filepath);

    // b. Method: Converts normalized pixel coordinates (px, py) to a ray in world coordinates.
    // px and py are typically normalized from 0 to 1 across the image plane.
    Ray generateRay(float px, float py) const;

    // Getters for resolution (used by the main rendering loop)
    int getResolutionX() const { return m_resolution_x; }
    int getResolutionY() const { return m_resolution_y; }

private:
    // --- Camera Parameters read from file ---
    Vector3 m_location;
    Vector3 m_gaze_direction_hint; // World space direction the camera is looking
    Vector3 m_up_vector_hint;      // World space vector for 'up' hint
    double m_focal_length;
    double m_sensor_width;
    double m_sensor_height;
    int m_resolution_x;
    int m_resolution_y;

    // --- Camera Basis Vectors (Calculated and stored internally for efficiency) ---
    // These vectors form the orthonormal basis for the camera's local space in world coordinates.
    Vector3 m_camera_u; // The calculated 'Right' vector
    Vector3 m_camera_v; // The calculated 'True Up' vector
    Vector3 m_camera_w; // The calculated 'Gaze/Forward' vector (normalized)

    // Helper method to parse the scene file
    void readSceneFile(const std::string& filepath);

    // Helper method to compute the orthonormal basis from the hints
    void computeCameraBasis();
};

#endif //B216602_CAMERA_H
