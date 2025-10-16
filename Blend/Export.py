
import bpy
import mathutils
import os
from math import pi

def format_vector(vec):
    """
    Formats a vector object into a string 'x y z'.
    INPUT: a vector object
    OUTPUT: a string
    """
    return f"{vec.x:.6f} {vec.y:.6f} {vec.z:.6f}"

def radians_to_deg(rotation_string):
    """
    Converts a string of three radian values (e.g., "1.16 -0.37 0.95")
    into a string of three degree values.
    INPUT: A string containing three space-separated radian values.
    OUTPUT: A string containing three space-separated degree values.
    """
    # 1. Convert the input string back into a list of floats
    rad_values = [float(x) for x in rotation_string.split()]

    # 2. Convert each radian value to degrees
    deg_values = [(rad * 180.0 / pi) for rad in rad_values]

    # 3. Format the degree values back into a space-separated string
    return f"{deg_values[0]:.6f} {deg_values[1]:.6f} {deg_values[2]:.6f}"

def export_scene_data(filepath):
    """
    Exports camera, point light, sphere, cube, and plane data from the current
    Blender scene to a text file.
    INPUT: a .blend scene
    OUTPUT: a .txt file
    """
    try:
        with open(filepath, 'w') as f:
            # Get the active scene
            scene = bpy.context.scene

            # --- Iterate through all objects in the scene ---
            for obj in scene.objects:

                # --- EXPORT CAMERA ---
                if obj.type == 'CAMERA':
                    cam_data = obj.data
                    f.write("CAMERA\n")
                    # Provides the X, Y, Z coordinates
                    f.write(f"  location {format_vector(obj.location)}\n")

                    # Calculate gaze direction as a unit vector that defines the direction the camera is looking
                    # It defines this by its vector components in the forward direction in X, Y, Z
                    inv_matrix = obj.matrix_world.copy()
                    gaze_vector = -inv_matrix.to_quaternion().to_matrix().col[2]
                    f.write(f"  gaze_direction {format_vector(gaze_vector)}\n")

                    # A camera's local +Y axis is its Up vector. It defines which way is up for the camera in reference to the world frame.
                    # Extract the second column (index 1) of the rotation matrix component.
                    up_vector = obj.matrix_world.to_3x3().col[1]
                    f.write(f"  up_vector {format_vector(up_vector)}\n")
                    # The camera's Euler rotation in World Space (XYZ order, radians)
                    # I provide this in addition to gaze and up for visualisation/testing, but use the gaze and up vectors in my raytracer
                    rotation_euler = format_vector(obj.rotation_euler)
                    f.write(f"  rotation_euler_radians {rotation_euler}\n")
                    f.write(f"  rotation_euler_degrees {radians_to_deg(rotation_euler)}\n")

                    f.write(f"  focal_length {cam_data.lens:.6f}\n")
                    f.write(f"  sensor_size {cam_data.sensor_width:.6f} {cam_data.sensor_height:.6f}\n")
                    f.write(f"  resolution {scene.render.resolution_x} {scene.render.resolution_y}\n")
                    f.write("END_CAMERA\n\n")

                # --- EXPORT POINT LIGHT ---
                elif obj.type == 'LIGHT' and obj.data.type == 'POINT':
                    light_data = obj.data
                    f.write("POINT_LIGHT\n")
                    # Provides the X, Y, Z coordinates
                    f.write(f"  location {format_vector(obj.location)}\n")
                    # In Blender, 'energy' is the power in Watts.
                    f.write(f"  intensity {light_data.energy:.6f}\n")
                    f.write("END_POINT_LIGHT\n\n")

                # --- EXPORT MESHES ---
                elif obj.type == 'MESH':
                    obj_name_lower = obj.name.lower()

                    # --- EXPORT SPHERE ---
                    # Assumes the object name contains "sphere"
                    if 'sphere' in obj_name_lower:
                        f.write("SPHERE\n")
                        # Provides the X, Y, Z coordinates
                        f.write(f"  location {format_vector(obj.location)}\n")
                        # Assumes uniform scaling for radius. This retrieves the X scale.
                        f.write(f"  radius {obj.scale.x:.6f}\n")
                        f.write("END_SPHERE\n\n")

                    # --- EXPORT CUBE ---
                    # Assumes the object name contains "cube"
                    elif 'cube' in obj_name_lower:
                        f.write("CUBE\n")
                        # Provides the X, Y, Z coordinates
                        f.write(f"  translation {format_vector(obj.location)}\n")
                        # Export rotation in Euler angles (XYZ order in radians)
                        rotation_euler = format_vector(obj.rotation_euler)
                        f.write(f"  rotation_euler_radians {rotation_euler}\n")
                        f.write(f"  rotation_euler_degrees {radians_to_deg(rotation_euler)}\n")
                        # Assumes uniform scaling, as requested for 1D scale.
                        f.write(f"  scale {obj.scale.x:.6f}\n")
                        f.write("END_CUBE\n\n")

                    # --- EXPORT PLANE ---
                    # Assumes the object name contains "plane"
                    elif 'plane' in obj_name_lower:
                        # Ensure the object has vertices
                        if obj.data.vertices:
                            f.write("PLANE\n")
                            # Get world coordinates for each vertex
                            for vertex in obj.data.vertices:
                                world_coord = obj.matrix_world @ vertex.co
                                f.write(f"  corner {format_vector(world_coord)}\n")
                            f.write("END_PLANE\n\n")

        print(f"Scene successfully exported to: {filepath}")
        return True

    except Exception as e:
        print(f"An error occurred during export: {e}")
        return False

# --- MAIN EXECUTION ---
if __name__ == "__main__":
    blend_file_path = bpy.context.blend_data.filepath

    if not blend_file_path:
        print("Error: Blender file must be saved before running the exporter.")
        output_filepath = "scene.txt"
    else:
        blend_dir = os.path.dirname(blend_file_path)

    project_root = os.path.dirname(blend_dir)

    ascii_dir = os.path.join(project_root, "ASCII")

    if not os.path.exists(ascii_dir):
        os.makedirs(ascii_dir)

    output_filename = "scene.txt"

    output_filepath = os.path.join(ascii_dir, output_filename)

    export_scene_data(output_filepath)