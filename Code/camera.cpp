//
// Created by alex on 16/10/2025.
//

#include "camera.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include "vector3.h"
#include <cstdlib>

inline Vector3 random_in_unit_disk() {
    while (true) {
        auto p = Vector3 ( (rand() / (RAND_MAX + 1.0)) * 2.0 - 1.0,
                            (rand() / (RAND_MAX + 1.0)) * 2.0 - 1.0,
                            0.0);
        if (p.dot(p) < 1.0)
            return p;
    }
}


Camera::Camera(
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
) : m_location(location),
      m_gaze_direction_hint(gaze_direction_hint),
      m_up_vector_hint(up_vector_hint),
      m_focal_length(focal_length),
      m_sensor_width(sensor_width),
      m_sensor_height(sensor_height),
      m_resolution_x(resolution_x),
      m_resolution_y(resolution_y),
      m_focal_distance(focal_distance)
{
    // compute aperture
    double focal_length_metres = m_focal_length / 1000.0;
    if (f_stop > 0) {
        m_aperture_radius = focal_length_metres / (2.0 * f_stop);
    }
    else {
        m_aperture_radius = 0.0;
    }
    // Compute the basis vectors.
    computeCameraBasis();
}

// Helper to compute the Camera's Orthonormal Basis (The LookAt transformation)

void Camera::computeCameraBasis() {
    // Calculate W (The Gaze/Forward vector)
    // The gaze direction is the forward vector, W. Normalize it to work with raytracing equations.
    m_camera_w = m_gaze_direction_hint.normalize();

    // Calculate U (The Right vector)
    // U is perpendicular to both the UP hint and the Gaze direction.
    // U = W x UP_HINT
    // U is right, V is up, and W is forward:
    // uses the UP_HINT x W cross product, then normalize.
    m_camera_u = m_up_vector_hint.cross(m_camera_w).normalize();

    // Calculate V (The True Up vector)
    // V must be perpendicular to both U and W to form a perfect orthonormal basis.
    // V = U x W
    m_camera_v = m_camera_u.cross(m_camera_w);
}

// Convert pixel coordinates to a ray in world coordinates.

Ray Camera::generateRay(float px, float py, double time) const {

    // Map pixel coordinates (px, py) to the image plane

    // px and py are normalized coordinates
    // Center the coordinates so (0.5, 0.5) is the center of the image.
    // The image plane's size is determined by the sensor width/height.

    // Scale the normalized pixel coordinates (0 to 1) to the actual sensor/view plane dimensions.
    // Map (0,0) from the top-left of the pixel grid (raster space) to the center of the pixel,
    // Adjust the y-axis (raster Y is down, world Y is usually up).

    // Calculate fractional pixel size relative to the sensor size
    double pixel_width_m = m_sensor_width / m_resolution_x;
    double pixel_height_m = m_sensor_height / m_resolution_y;

    double image_aspect_ratio = static_cast<double>(m_resolution_x) / static_cast<double>(m_resolution_y);
    double effective_sensor_height = m_sensor_width / image_aspect_ratio;

    // Calculate the coordinate on the sensor plane relative to the center of the sensor (in camera's local U/V space)

    // includes flipping the image
    double u_coord = (0.5 - px) * m_sensor_width;
    double v_coord = (py - 0.5) * m_sensor_height;

    Vector3 pinhole_dir = (m_focal_length * m_camera_w) +
                          (u_coord * m_camera_u) +
                          (v_coord * m_camera_v);
    pinhole_dir = pinhole_dir.normalize();

    if (m_aperture_radius <= 0.0) {
        return Ray(m_location, pinhole_dir, time);
    }

    Vector3 focal_point = m_location + pinhole_dir * m_focal_distance;

    Vector3 random_disk_pt = random_in_unit_disk() * m_aperture_radius;
    Vector3 lens_offset = m_camera_u * random_disk_pt.x + m_camera_v * random_disk_pt.y;
    Vector3 ray_origin = m_location + lens_offset;

    Vector3 new_dir = (focal_point - ray_origin).normalize();

    return Ray(ray_origin, new_dir, time);
}
