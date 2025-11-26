
# Requirements

The base of this project exclusively uses the standard C++ library. You should be able to run it with just this. However, some extended features have additional requirements. Firstly, in order to enable `.jpeg`, `.jpg`, or `png` texture or bump map files, `Python` with `Pillow (PIL)` must be installed on your system. Secondly, to enable multi-threading you must have `OpenMP` installed. 

The system requires a minimum `CMake` version of `3.20`, and a `C++20` standard compiler.

# Features

* Module 1
  * Blender exporter.
  * Raytracing from a camera.
  * Image read and write in `.ppm` format.
* Module 2
  * Ray intersection for sphere, plane and cube objects.
  * Acceleration hierarchy using the bounding volume hierarchy.
* Module 3 
  * Whitted-Style ray tracing, shading intersections according to the Blinn-Phong model.
  * Traced refracted/reflected rays.
  * Antialiasing using average contributions of samples.
  * UV textures mapped to shapes.
* Final Raytracer 
  * Implementation of soft shadows via distributed raytracing.
  * Implementation of glossy reflection via distributed raytracing.
  * Motion blur.
  * Depth of field blur.
* Advanced Features
  * Fresnel effect.
  * `.jpeg`, `.jpg`, or `png` texture conversion.
  * Bump mapping.
  * Displacement mapping.
  * Metal material.
  * Multi-threading.

# Usage

The raytracer parses `ASCII/scene.txt` which contains the objects and object data exported from Blender. To export this data from Blender, load and run the `Blend/Export.py` in the `Blend/scene.py` file. Details on structuring the scene for parsing can be found in [Parameters](#parameters).

Once the `scene.txt` has been generated, the code can be executed. By default it will run without any command line arguments, however it can be tuned using the `Code/config.json` parameters and the command line arguments described in [Parameters](#parameters).   

The resulting image is saved as a `.ppm` file in `Output/scene_test.ppm`.

## Example Outputs

### Module 1

In module 1, the `ASCII/scene.txt` contains the following information:
* Cameras
  * Location
  * Direction of the gaze and up vectors
  * Focal length
  * Sensor width and height
  * Film resolution
* Point lights
  * Location
  * Radiant intensity
* Spheres
  * Location
  * Radius (1D)
* Cubes
  * Translation
  * Rotation
  * Scale (1D)
* Planes
  * 3D coordinates of its four corners
An example output of `ASCII/scene.txt` for module 1 can be found at `Report/examples/M1/scene.txt`. This was tested by manually entering the values in the text file into a new object in Blender, and checking that it overlaps with the existing object.
The camera class and header files can be found in `Code/environment`. 

# Timeliness

All deadlines were met, with the corresponding features implemented in each module.

## Deviations
### Module 1
This module involved a python exporter which took limited data about objects. For example, module 1 did not require any data about the material of the object and so future versions of the exporter has additional functionality to retrieve material data.

Additionally this module exported 1D scales for the shapes, whereas later versions retrieved values for scale in 3D dimensions. Similarly, the intensity of the light at this point is a 1D vector, whereas in future this was changed to a 3D vector to allow the light to be coloured. The values of this new vector represent the intensity of each colour channel.

Finally, the methods for reading the file from the exporter are in the camera class and header in M1. In future, they are moved to `Code/utilities/scene.cpp` and `Code/utilities/scene.h` for tidiness. 

# Parameters

| Parameter / Property    | Location / Method                                         | Description                                                                                                                                                                                                                                                                                                    |
|:------------------------|:----------------------------------------------------------|:---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **Global Settings**     |                                                           |                                                                                                                                                                                                                                                                                                                |
| `samples_per_pixel`     | `config.json`                                             | Default number of rays cast per pixel (Antialiasing). Higher is slower but smoother. This can be overridden using the `--aa <int>` flag.                                                                                                                                                                       |
| `max_bounces`           | `config.json`                                             | Maximum recursion depth for reflections/refractions.                                                                                                                                                                                                                                                           |
| `exposure`              | `config.json`                                             | Default brightness multiplier for the final image. This can be overridden using the `--exposure <float>` flag                                                                                                                                                                                                  |
| `shutter_time`          | `config.json`                                             | Default duration the shutter is open (used for motion blur calculations). This can be overridden using the `--motion-blur <float>` flag                                                                                                                                                                        |
| `shadow_samples`        | `config.json`                                             | Number of shadow rays cast per light source per hit (soft shadows).                                                                                                                                                                                                                                            |
| `glossy_samples`        | `config.json`                                             | Number of reflection rays scattered for rough surfaces.                                                                                                                                                                                                                                                        |
| `epsilon`               | `config.json`                                             | Small offset value to prevent self-shadowing acne.                                                                                                                                                                                                                                                             |
| `ray_march_steps`       | `config.json`                                             | Maximum iterations for ray marching complex shapes.                                                                                                                                                                                                                                                            |
| `displacement_strength` | `config.json`                                             | Intensity of displacement mapping on surfaces.                                                                                                                                                                                                                                                                 |
| **Command Line Flags**  |                                                           |                                                                                                                                                                                                                                                                                                                |
| `--aa <int>`            | Command Line                                              | Overrides `samples_per_pixel` from config.                                                                                                                                                                                                                                                                     |
| `--exposure <float>`    | Command Line                                              | Overrides `exposure` from config.                                                                                                                                                                                                                                                                              |
| `--motion-blur <float>` | Command Line                                              | Overrides `shutter_time` from config. Enables motion blur.                                                                                                                                                                                                                                                     |
| `--shadows`             | Command Line                                              | Enable shadow calculations (defaults to off).                                                                                                                                                                                                                                                                  |
| `--fresnel`             | Command Line                                              | Enable Fresnel equations for realistic reflection weighting.                                                                                                                                                                                                                                                   |
| `--parallel`            | Command Line                                              | Enables multi-threading (OpenMP) for faster rendering. If OpenMP is not available, the program will run with a single thread.                                                                                                                                                                                  |
| `--no-bvh`              | Command Line                                              | Disables the Bounding Volume Hierarchy (acceleration structure).                                                                                                                                                                                                                                               |
| `--time <int>`          | Command Line                                              | Runs the render `<int>` times and logs performance stats.                                                                                                                                                                                                                                                      |
| **Blender (Camera)**    |                                                           |                                                                                                                                                                                                                                                                                                                |
| `Location`              | Blender → Camera → Object → Location                      | 3D position of the camera in 3D space (`x, y, z`).                                                                                                                                                                                                                                                             |
| `Gaze Direction`        | Blender → Camera → Object → Rotation                      | 3D vector direction the camera is looking.                                                                                                                                                                                                                                                                     |
| `Up Vector`             | Blender → Camera → Object → Rotation                      | 3D vector indicating the "up" direction for camera orientation.                                                                                                                                                                                                                                                |
| `Focal Length`          | Blender → Camera → Data → Focal Length                    | Distance from the lens to the film/sensor.                                                                                                                                                                                                                                                                     |
| `Sensor Size`           | Blender → Camera → Data → Camera                          | Physical dimensions of the camera sensor.                                                                                                                                                                                                                                                                      |
| `Resolution`            | Blender → Scene → Resolution                              | Camera resolution.                                                                                                                                                                                                                                                                                             |
| `F-Stop`                | Blender → Camera → Data → Depth of Field → F-Stop         | Aperture size (controls depth of field blur).                                                                                                                                                                                                                                                                  |
| `Focal Distance`        | Blender → Camera → Data → Depth of Field → Focus Distance | Distance at which objects are perfectly in focus.                                                                                                                                                                                                                                                              |
| **Blender (Lights)**    |                                                           |                                                                                                                                                                                                                                                                                                                |
| `Location`              | Blender → Light → Object → Location                       | Position of the point light.                                                                                                                                                                                                                                                                                   |
| `Intensity`             | Blender → Light → Data → Colour x Power                   | RGB color/strength of the light.                                                                                                                                                                                                                                                                               |
| `Radius`                | Blender → Data → Custom Properties → light_radius         | Physical size of the light (affects shadow softness).                                                                                                                                                                                                                                                          |
| **Blender (Shapes)**    |                                                           |                                                                                                                                                                                                                                                                                                                |
| `Translation`           | Blender → Object → Location                               | Position offset of the object.                                                                                                                                                                                                                                                                                 |
| `Rotation`              | Blender → Object → Rotation                               | Euler rotation (radians) of the object.                                                                                                                                                                                                                                                                        |
| `Scale`                 | Blender → Object → Scale                                  | Size multiplier of the object.                                                                                                                                                                                                                                                                                 |
| `Velocity`              | Blender → Material → Custom Properties → velocity         | 3D movement vector for motion blur calculation.                                                                                                                                                                                                                                                                |
| `Material (Ambient)`    | Blender → Material → Custom Properties → ambient          | 3D vector base color component (shadow areas). Range of (0.0 to 1.0) per colour channel. Values are typically 10% of the corresponding diffuse channel.                                                                                                                                                        |
| `Material (Diffuse)`    | Blender → Material → Custom Properties → diffuse          | 3D vector main surface color component. Range of (0.0 to 1.0) per colour channel.                                                                                                                                                                                                                              |
| `Material (Specular)`   | Blender → Material → Custom Properties → specular         | 3D vector highlight color component. A lower value creates a more matt surface.                                                                                                                                                                                                                                |
| `Shininess`             | Blender → Material → Custom Properties → shininess        | Tightness of specular highlights. Higher values create smaller, sharper highlights. Range of (0.0 to inf).                                                                                                                                                                                                     |
| `Reflectivity`          | Blender → Material → Custom Properties → reflectivity     | Mirror-like quality (0.0 to 1.0). 1.0 is a perfect mirror. Reflectivity and transparency must add to less than or equal to 1.0.                                                                                                                                                                                |
| `Transparency`          | Blender → Material → Custom Properties → transparency     | Light transmission capability (0.0 to 1.0). 0.0 is full opaque, 1.0 is transparent. Reflectivity and transparency must add to less than or equal to 1.0.                                                                                                                                                       |
| `Refractive Index`      | Blender → Material → Custom Properties → refractive_index | Optical density (e.g., 1.5 for glass, 1.33 for water). Range (1.0, inf)                                                                                                                                                                                                                                        |
| `Material`              | Blender → Material → Custom Properties → material         | This property can be `glass` or `metal`. Reflective metal objects will tint their reflection with the colour of the metal.                                                                                                                                                                                     |
| `Texture File`          | Blender → Material → Custom Properties → texture_file     | Filename of the texture map to apply. If python is installed, then texture files can be `.ppm`, `.jpeg`, `.jpg`, or `png`. Otherwise, texture files must be `.ppm`.                                                                                                                                            |
| `Bump Map File`         | Blender → Material → Custom Properties → bump_map_file    | Filename of the bump map for surface detail. The addition of the `complex_` keyword to a shapes name will result in this map being used to displace the geometry of the shape. If python is installed, then bump map files can be `.ppm`, `.jpeg`, `.jpg`, or `png`. Otherwise, bump map files must be `.ppm`. |


# Theory