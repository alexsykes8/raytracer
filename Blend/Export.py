
import bpy
import mathutils

def format_vector(vec):
    """Formats a vector object into a string 'x y z'."""
    return f"{vec.x:.6f} {vec.y:.6f} {vec.z:.6f}"

def export_scene_data(filepath):
    """
    Exports camera, point light, sphere, cube, and plane data from the current
    Blender scene to a text file.
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
                    f.write(f"  location {format_vector(obj.location)}\n")

                    # Calculate gaze direction
                    # A camera points along its local -Z axis.
                    # Transform this vector into world space.
                    inv_matrix = obj.matrix_world.copy()
                    gaze_vector = -inv_matrix.to_quaternion().to_matrix().col[2]
                    f.write(f"  gaze_direction {format_vector(gaze_vector)}\n")

                    f.write(f"  focal_length {cam_data.lens:.6f}\n")
                    f.write(f"  sensor_size {cam_data.sensor_width:.6f} {cam_data.sensor_height:.6f}\n")
                    f.write(f"  resolution {scene.render.resolution_x} {scene.render.resolution_y}\n")
                    f.write("END_CAMERA\n\n")

                # --- EXPORT POINT LIGHT ---
                elif obj.type == 'LIGHT' and obj.data.type == 'POINT':
                    light_data = obj.data
                    f.write("POINT_LIGHT\n")
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
                        f.write(f"  location {format_vector(obj.location)}\n")
                        # Assumes uniform scaling for radius. We use the X scale.
                        f.write(f"  radius {obj.scale.x:.6f}\n")
                        f.write("END_SPHERE\n\n")

                    # --- EXPORT CUBE ---
                    # Assumes the object name contains "cube"
                    elif 'cube' in obj_name_lower:
                        f.write("CUBE\n")
                        f.write(f"  translation {format_vector(obj.location)}\n")
                        # Export rotation in Euler angles (XYZ order in radians)
                        f.write(f"  rotation_euler {format_vector(obj.rotation_euler)}\n")
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
    output_filepath = "scene.txt"

    export_scene_data(output_filepath)