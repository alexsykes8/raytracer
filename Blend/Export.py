import bpy
import mathutils
import os
from math import pi

def write_material_properties(f, obj):
    """
    Helper function to write custom material properties if they exist.
    It accesses properties from the object's active material.
    """
    
    if not obj.active_material:
        return # No material, so no properties to write

    mat = obj.active_material

    if "ambient" in mat:
        ambient_vec = mathutils.Vector(mat['ambient'])
        f.write(f"  ambient {format_vector(ambient_vec)}\n")

    if "diffuse" in mat:
        diffuse_vec = mathutils.Vector(mat['diffuse'])
        f.write(f"  diffuse {format_vector(diffuse_vec)}\n")

    if "specular" in mat:
        specular_vec = mathutils.Vector(mat['specular'])
        f.write(f"  specular {format_vector(specular_vec)}\n")

    if "shininess" in mat:
        f.write(f"  shininess {mat['shininess']:.6f}\n")

    if "reflectivity" in mat:
        f.write(f"  reflectivity {mat['reflectivity']:.6f}\n")
        
    if "transparency" in mat:
        f.write(f"  transparency {mat['transparency']:.6f}\n")

    if "refractive_index" in mat:
        f.write(f"  refractive_index {mat['refractive_index']:.6f}\n")

    if "texture_file" in mat:
        texture_name = mat['texture_file']
        if texture_name:
            cleaned_name = texture_name.replace('//', '').replace('\\', '/')
            f.write(f"  texture_file {cleaned_name}\n")

    if "bump_map_file" in mat:
        texture_name = mat['bump_map_file']
        if texture_name:
            cleaned_name = texture_name.replace('//', '').replace('\\', '/')
            f.write(f"  bump_map_file {cleaned_name}\n")

    if "velocity" in mat:
        velocity_vec = mathutils.Vector(mat['velocity'])
        f.write(f"  velocity {format_vector(velocity_vec)}\n")


def get_primitive_type(obj):
    mesh = obj.data

    # --- Plane Check ---
    # A default plane has: 4 vertices, 4 edges, 1 face
    if len(mesh.vertices) == 4 and len(mesh.edges) == 4 and len(mesh.polygons) == 1:
        return "PLANE"

    # --- Cube Check ---
    # A default cube has: 8 vertices, 12 edges, 6 faces
    if len(mesh.vertices) == 8 and len(mesh.edges) == 12 and len(mesh.polygons) == 6:
        # Additional check: all faces are quads (4 vertices per face)
        is_quad_cube = all(poly.loop_total == 4 for poly in mesh.polygons)
        if is_quad_cube:
            if "complex" in obj.name.lower():
                return "COMPLEX_CUBE"
            return "CUBE"


    # --- Assume if not a cube or plane then it is a sphere
    if "complex" in obj.name.lower():
        return "COMPLEX_SPHERE"
    return "SPHERE"


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

            # Export HDR world properties
            world = scene.world
            if world and "HDR_BACKGROUND" in world:
                hdr_path = world["HDR_BACKGROUND"]
                if hdr_path:
                    cleaned_path = hdr_path.replace('//', '').replace('\\', '/')
                    f.write(f"HDR_BACKGROUND {cleaned_path}\n\n")

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

                    if cam_data.dof.use_dof:
                        f.write(f"  f_stop {cam_data.dof.aperture_fstop:.6f}\n")
                        f.write(f"  focal_distance {cam_data.dof.focus_distance:.6f}\n")
                    else:
                        f.write(f"  f_stop 99999.0\n")
                        f.write(f"  focal_distance 10.0\n")
                    f.write("END_CAMERA\n\n")

                # --- EXPORT POINT LIGHT ---
                elif obj.type == 'LIGHT' and obj.data.type == 'POINT':
                    light_data = obj.data
                    f.write("POINT_LIGHT\n")
                    # Provides the X, Y, Z coordinates
                    f.write(f"  location {format_vector(obj.location)}\n")
                    # In Blender, 'energy' is the power in Watts.
                    energy = light_data.energy
                    colour = light_data.color
                    # the final intensity is the energy multiplied by the colour
                    f.write(f"  intensity {colour[0] * energy:.6f} {colour[1] * energy:.6f} {colour[2] * energy:.6f}\n")
                    radius = 0.0
                    if "radius" in obj:
                        radius = obj["radius"]
                    elif "light_radius" in obj:
                        radius = obj["light_radius"]
                    elif "radius" in light_data:
                        radius = light_data["radius"]
                    elif "light_radius" in light_data:
                        radius = light_data["light_radius"]
                    f.write(f"  radius {radius:.6f}\n")
                    f.write("END_POINT_LIGHT\n\n")

                # --- EXPORT MESHES ---
                elif obj.type == 'MESH':
                    object_type = get_primitive_type(obj)

                    # --- EXPORT SPHERE (NOW HANDLES NON-UNIFORM SCALE) ---
                    if object_type == "SPHERE":
                        f.write("SPHERE\n")
                        # Use 'translation' for consistency with CUBE
                        f.write(f"  translation {format_vector(obj.location)}\n")
                        # Export rotation in Euler angles (XYZ order in radians)
                        rotation_euler = format_vector(obj.rotation_euler)
                        f.write(f"  rotation_euler_radians {rotation_euler}\n")
                        f.write(f"  rotation_euler_degrees {radians_to_deg(rotation_euler)}\n")
                        # Export non-uniform scaling (X, Y, Z)
                        f.write(f"  scale {format_vector(obj.scale)}\n")
                        write_material_properties(f, obj)
                        f.write("END_SPHERE\n\n")

                    elif object_type == "COMPLEX_SPHERE":
                        f.write("COMPLEX_SPHERE\n")
                        # Use 'translation' for consistency with CUBE
                        f.write(f"  translation {format_vector(obj.location)}\n")
                        # Export rotation in Euler angles (XYZ order in radians)
                        rotation_euler = format_vector(obj.rotation_euler)
                        f.write(f"  rotation_euler_radians {rotation_euler}\n")
                        f.write(f"  rotation_euler_degrees {radians_to_deg(rotation_euler)}\n")
                        # Export non-uniform scaling (X, Y, Z)
                        f.write(f"  scale {format_vector(obj.scale)}\n")
                        write_material_properties(f, obj)
                        f.write("END_COMPLEX_SPHERE\n\n")

                    # --- EXPORT CUBE ---
                    elif object_type == "CUBE":
                        f.write("CUBE\n")
                        # Provides the X, Y, Z coordinates
                        f.write(f"  translation {format_vector(obj.location)}\n")
                        # Export rotation in Euler angles (XYZ order in radians)
                        rotation_euler = format_vector(obj.rotation_euler)
                        f.write(f"  rotation_euler_radians {rotation_euler}\n")
                        f.write(f"  rotation_euler_degrees {radians_to_deg(rotation_euler)}\n") # used for visual testing
                        # Assumes uniform scaling, as requested for 1D scale.
                        f.write(f"  scale {format_vector(obj.scale)}\n")
                        write_material_properties(f, obj)
                        f.write("END_CUBE\n\n")

                    elif object_type == "COMPLEX_CUBE":
                        f.write("COMPLEX_CUBE\n")
                        f.write(f"  translation {format_vector(obj.location)}\n")
                        rotation_euler = format_vector(obj.rotation_euler)
                        f.write(f"  rotation_euler_radians {rotation_euler}\n")
                        f.write(f"  rotation_euler_degrees {radians_to_deg(rotation_euler)}\n")
                        f.write(f"  scale {format_vector(obj.scale)}\n")
                        write_material_properties(f, obj)
                        f.write("END_COMPLEX_CUBE\n\n")

                    # --- EXPORT PLANE ---
                    elif object_type == "PLANE":
                        # Ensure the object has vertices
                        if obj.data.vertices:
                            f.write("PLANE\n")
                            # Get world coordinates for each vertex
                            for vertex in obj.data.vertices:
                                world_coord = obj.matrix_world @ vertex.co
                                f.write(f"  corner {format_vector(world_coord)}\n")
                            write_material_properties(f, obj)
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