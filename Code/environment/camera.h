//
// Created by alex on 16/10/2025.
//

#ifndef B216602_CAMERA_H
#define B216602_CAMERA_H


#include <string>
#include <stdexcept>
#include "../utilities/vector3.h"
#include "../utilities/ray.h"
#include <cmath>

// it handles reading camera parameters and performing coordinate transformations.
class Camera {
public:
    // constructor that initialises the camera with its position, orientation, and lens properties.
    Camera(
            Vector3 location,
            Vector3 gaze_direction_hint,
            Vector3 up_vector_hint,
            double focal_length,
            double sensor_width,
            double sensor_height,
            int resolution_x,
            int resolution_y,
            double f_stop,
            double focal_distance
        );
    // converts normalised pixel coordinates (px, py) to a ray in world coordinates, accounting for depth of field and motion blur.
    Ray generateRay(float px, float py, double time = 0.0) const;

    // returns the horizontal resolution of the camera in pixels.
    int getResolutionX() const { return m_resolution_x; }
    // returns the vertical resolution of the camera in pixels.
    int getResolutionY() const { return m_resolution_y; }

private:
    // the world-space position of the camera.
    Vector3 m_location;
    // a hint for the world-space direction the camera is looking.
    Vector3 m_gaze_direction_hint;
    // a hint for the world-space 'up' direction.
    Vector3 m_up_vector_hint;
    // the distance from the pinhole/lens to the sensor plane, in millimeters.
    double m_focal_length;
    // the width of the camera's sensor, in millimeters.
    double m_sensor_width;
    // the height of the camera's sensor, in millimeters.
    double m_sensor_height;
    // the horizontal resolution of the output image, in pixels.
    int m_resolution_x;
    // the vertical resolution of the output image, in pixels.
    int m_resolution_y;

    // the radius of the lens aperture, calculated from the f-stop and focal length.
    double m_aperture_radius;
    // the distance from the camera to the plane that is perfectly in focus.
    double m_focal_distance;

    // the calculated 'right' vector of the camera's orthonormal basis.
    Vector3 m_camera_u;
    // the calculated 'true up' vector of the camera's orthonormal basis.
    Vector3 m_camera_v;
    // the calculated 'forward' (negative gaze) vector of the camera's orthonormal basis.
    Vector3 m_camera_w;

    // a helper method to compute the orthonormal basis (u, v, w) from the provided hint vectors.
    void computeCameraBasis();
};

#endif //B216602_CAMERA_H
