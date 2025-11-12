#pragma once

#include "device_types.cuh"
#include <string>
#include <stdexcept>

class Camera {
public:
    Camera(
            Vector3 location,
            Vector3 gaze_direction_hint,
            Vector3 up_vector_hint,
            double focal_length,
            double sensor_width,
            double sensor_height,
            int resolution_x,
            int resolution_y
        );

    Ray generateRay(float px, float py) const;

    int getResolutionX() const { return m_resolution_x; }
    int getResolutionY() const { return m_resolution_y; }

    double getFocalLength() const { return m_focal_length; }
    double getSensorWidth() const { return m_sensor_width; }
    double getSensorHeight() const { return m_sensor_height; }

    const Vector3& getLocation() const { return m_location; }
    const Vector3& getU() const { return m_camera_u; } // "Right"
    const Vector3& getV() const { return m_camera_v; } // "True Up"
    const Vector3& getW() const { return m_camera_w; } // "Gaze"

private:
    // Camera Parameters read from file
    Vector3 m_location;
    Vector3 m_gaze_direction_hint; // World space direction the camera is looking
    Vector3 m_up_vector_hint;      // World space vector for 'up' hint
    double m_focal_length;
    double m_sensor_width;
    double m_sensor_height;
    int m_resolution_x;
    int m_resolution_y;

    // Camera Basis Vectors
    Vector3 m_camera_u; // The calculated 'Right' vector
    Vector3 m_camera_v; // The calculated 'True Up' vector
    Vector3 m_camera_w; // The calculated 'Gaze/Forward' vector

    void computeCameraBasis();
};
