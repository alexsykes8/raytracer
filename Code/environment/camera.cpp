//
// Created by alex on 16/10/2025.
//

#include "camera.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include "../utilities/vector3.h"
#include <cstdlib>

// generates a random point inside a 2d unit disk on the xy-plane.
inline Vector3 random_in_unit_disk() {
    // uses rejection sampling to find a point within the disk.
    while (true) {
        // generates a random point within a 2x2 square centered at the origin.
        auto p = Vector3 ( (rand() / (RAND_MAX + 1.0)) * 2.0 - 1.0,
                            (rand() / (RAND_MAX + 1.0)) * 2.0 - 1.0,
                            0.0);
        // if the point's squared length is less than 1, it is inside the unit disk.
        if (p.dot(p) < 1.0)
            return p;
    }
}

// constructor that initialises the camera with its position, orientation, and lens properties.
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
      // initialises member variables from the constructor parameters.
      m_gaze_direction_hint(gaze_direction_hint),
      m_up_vector_hint(up_vector_hint),
      m_focal_length(focal_length),
      m_sensor_width(sensor_width),
      m_sensor_height(sensor_height),
      m_resolution_x(resolution_x),
      m_resolution_y(resolution_y),
      m_focal_distance(focal_distance)
{
    // converts focal length from millimeters to meters for aperture calculation.
    // equation: focal_length_metres = m_focal_length / 1000.0
    double focal_length_metres = m_focal_length / 1000.0;
    // calculates the lens aperture radius if f-stop is positive.
    if (f_stop > 0) {
        // equation: aperture_radius = focal_length_metres / (2.0 * f_stop)
        m_aperture_radius = focal_length_metres / (2.0 * f_stop);
    }
    else {
        // if f-stop is zero or negative, simulate a perfect pinhole camera with no aperture.
        m_aperture_radius = 0.0;
    }
    // computes the camera's local coordinate system (u, v, w vectors).
    computeCameraBasis();
}

// computes the camera's orthonormal basis (the lookat transformation).
void Camera::computeCameraBasis() {
    // calculates w (the forward vector), which is the opposite of the normalised gaze direction.
    // equation: w = -gaze_direction / |gaze_direction|
    m_camera_w = m_gaze_direction_hint.normalize();

    // calculates u (the right vector), which is perpendicular to the up hint and the forward vector.
    // equation: u = (up_hint x w) / |up_hint x w|
    m_camera_u = m_up_vector_hint.cross(m_camera_w).normalize();

    // calculates v (the true up vector), which is perpendicular to u and w, completing the orthonormal basis.
    // equation: v = u x w
    m_camera_v = m_camera_u.cross(m_camera_w);
}

// converts normalised pixel coordinates to a ray in world coordinates.
Ray Camera::generateRay(float px, float py, double time) const {

    // maps normalised pixel coordinates (px, py in [0,1]) to sensor plane coordinates.

    // calculates the horizontal coordinate on the sensor plane.
    // (0.5 - px) centers the coordinate system and flips it to match a right-handed coordinate system.
    // equation: u_coord = (0.5 - px) * m_sensor_width
    double u_coord = (0.5 - px) * m_sensor_width;
    // calculates the vertical coordinate on the sensor plane.
    // (py - 0.5) centers the coordinate system.
    // equation: v_coord = (py - 0.5) * m_sensor_height
    double v_coord = (py - 0.5) * m_sensor_height;

    // calculates the direction vector from the pinhole to the point on the sensor plane.
    // this is done in the camera's local coordinate system then implicitly transformed to world space.
    // equation: pinhole_dir = (focal_length * w) + (u_coord * u) + (v_coord * v)
    Vector3 pinhole_dir = (m_focal_length * m_camera_w) +
                          (u_coord * m_camera_u) +
                          (v_coord * m_camera_v);
    // normalises the direction vector.
    pinhole_dir = pinhole_dir.normalize();

    // if aperture is zero, simulate a perfect pinhole camera.
    if (m_aperture_radius <= 0.0) {
        return Ray(m_location, pinhole_dir, time);
    }

    // simulates a thin lens for depth of field.
    // calculates the point on the focal plane that the pinhole ray intersects.
    // equation: focal_point = camera_location + pinhole_dir * focal_distance
    Vector3 focal_point = m_location + pinhole_dir * m_focal_distance;

    // generates a random point on the lens (a disk with radius m_aperture_radius).
    Vector3 random_disk_pt = random_in_unit_disk() * m_aperture_radius;
    // maps the 2d disk point to the 3d lens plane using the camera's u and v basis vectors.
    Vector3 lens_offset = m_camera_u * random_disk_pt.x + m_camera_v * random_disk_pt.y;
    // calculates the new ray origin on the lens surface.
    // equation: ray_origin = camera_location + lens_offset
    Vector3 ray_origin = m_location + lens_offset;

    // calculates the new ray direction from the point on the lens to the focal point.
    // equation: new_dir = (focal_point - ray_origin) normalised
    Vector3 new_dir = (focal_point - ray_origin).normalize();

    // returns the final ray for depth of field.
    return Ray(ray_origin, new_dir, time);
}
