//
// Created by alex on 16/10/2025.
//

#include "camera.h"
#include <fstream>
#include <sstream>
#include <iostream>


Camera::Camera(
    Vector3 location,
    Vector3 gaze_direction_hint,
    Vector3 up_vector_hint,
    double focal_length,
    double sensor_width,
    double sensor_height,
    int resolution_x,
    int resolution_y
) : m_location(location),
      m_gaze_direction_hint(gaze_direction_hint),
      m_up_vector_hint(up_vector_hint),
      m_focal_length(focal_length),
      m_sensor_width(sensor_width),
      m_sensor_height(sensor_height),
      m_resolution_x(resolution_x),
      m_resolution_y(resolution_y)
{
    // After storing parameters, compute the basis vectors.
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

Ray Camera::generateRay(float px, float py) const {

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

    // Calculate the coordinate on the sensor plane relative to the center of the sensor (in camera's local U/V space)

    // includes flipping the image
    double u_coord = (0.5 - px) * m_sensor_width;
    double v_coord = (py - 0.5) * m_sensor_height;

    // Calculate the Ray Direction in World Space

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
    // the point on the image plane is the direction vector.
    Vector3 ray_direction = image_plane_point.normalize();

    // Construct the Ray
    Ray r;
    r.origin = m_location;
    r.direction = ray_direction; // Already normalized

    return r;
}
