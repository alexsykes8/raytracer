
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
* Exceptionality
  * Multi-threading.
  * Fresnel effect.
  * `.jpeg`, `.jpg`, or `png` texture conversion.
  * HDR background images.
  * Bump mapping.
  * Displacement mapping.
  * Metal material.
  * Exposure control.
  * Tone mapping (interchangeable Reinhard, ACES, and Filmic)

# Usage

The raytracer parses `ASCII/scene.txt` which contains the objects and object data exported from Blender. To export this data from Blender, load and run the `Blend/Export.py` in the `Blend/scene.py` file. Details on structuring the scene for parsing can be found in [Parameters](#parameters).

Once the `scene.txt` has been generated, the code can be executed. By default it will run without any command line arguments, however it can be tuned using the `Code/config.json` parameters and the command line arguments described in [Parameters](#parameters).   

The resulting image is saved as a `.ppm` file in `Output/scene_test.ppm`.

## Example Outputs

### Module 1

#### Blender exporter

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

#### Camera space transformations
The camera class and header files can be found in `Code/environment/camera.h` and `Code/environment/camera.cpp`. In module 1, the file is read by the camera class and information stored. A vector3 class and a ray class are implemented to structure the data.

#### Image read and write

The functionality for image reading and writing can be found in `Code/utilities/scene.h` and `Code/utilities/scene.cpp`. This was tested by reading a `.ppm` file of a generic gradient, replacing some pixels' colour values with white, and writing it back to a `.ppm`.
<table style="width: 100%; border: none;">
  <tr>
    <td style="width: 50%; padding: 10px; text-align: center; border: none;">
      <img src="Report/examples/M1/gradient.png" alt="Figure A" style="width: 100%;">
      <p style="text-align: center;"><b>Figure 1:</b> Read file</p>
    </td>
    <td style="width: 50%; padding: 10px; text-align: center; border: none;">
      <img src="Report/examples/M1/modified.png" alt="Figure B" style="width: 100%;">
      <p style="text-align: center;"><b>Figure 2:</b> Write file</p>
    </td>
  </tr>
</table>

### Module 2

#### Ray Intersection

Module 2 was tested by overlaying the original Blender file with the output from the raytracer and checking that they match. To generate the output, I coloured the ray intersections according to the normal of the ray with the object.

<table style="width: 100%; border: none;">
  <tr>
    <td style="width: 50%; padding: 10px; text-align: center; border: none;">
      <img src="Report/examples/M2/scene_test_white.png" alt="Figure A" style="width: 100%;">
      <p style="text-align: center;"><b>Figure 1: </b>Generated image</p>
    </td>
    <td style="width: 50%; padding: 10px; text-align: center; border: none;">
      <img src="Report/examples/M2/blender_white.png" alt="Figure B" style="width: 100%;">
      <p style="text-align: center;"><b>Figure 2: </b>Original Blender scene</p>
    </td>
  </tr>
</table>

#### Acceleration Hierarchy

A bounding volume hierarchy is implemented to improve the efficiency of intersection tests for scenes with many shapes. The speedup for the scene in [Figure 1](#figure-1) is shown in [Figure 2](#figure-2). Each test was run 5 times and averaged. As shown, the improvement in efficiency increases as the number of items in the scene increases.


<table style="width: 100%; border: none;">
  <tr>
    <td style="width: 50%; padding: 10px; text-align: center; border: none;">
      <a id="figure-1"></a>
      <img src="" alt="Figure A" style="width: 100%;">
      <p style="text-align: center;"><b>Figure 1: </b>Raytraced scene</p>
    </td>
    <td style="width: 50%; padding: 10px; text-align: center; border: none;">
      <a id="figure-2"></a>
      <img src="" alt="Figure B" style="width: 100%;">
      <p style="text-align: center;"><b>Figure 2: </b>Runtime with and without BVH</p>
    </td>
  </tr>
</table>

### Exceptionality

#### Tonemapping

Tonemapping was implemented as an optional `--tonemap <algorithm>` flag. The implemented algorithms are:
* Reinhard: this uses the function $C_d = \frac{C}{1+C}$ where $C$ is the colour vector.
* ACES: this uses an S-shaped contrast curve. The values are calculated using
  [Krzysztof Narkowicz's](https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/) values in the S-curve equation $\frac{x  (a  x + b)}{x  (c  x + d) + e}$ where x is the value of a colour channel.
* Filmic: this algorithm applies the equation $\frac{(x (ax + cb) + d e) }{x(ax + b) + d f)} - \frac{e}{f}$ with values from [John Hable](http://filmicworlds.com/blog/filmic-tonemapping-operators/), where x is the value of a colour channel. 

The algorithms are applied to each pixel after the colour has been calculated to map the HDR colour range to a limited range. Without the inclusion of this flag, the values are simply clamped to the range of 0.0 to 1.0.

While the values of the pixels can be controlled with the `--exposure <float>` flag, this scales the values of the pixels linearly. Tonemapping adjusts the values more intelligently, for example crushing highlights or shadows more aggressively than the midtones. Exposure and tonemapping can be used in tandem to control the general brightness of the scene with artistic choices about how the light should be interpreted.  

<table style="width: 100%; border: none;">
  <tr>
    <td style="width: 50%; padding: 10px; text-align: center; border: none;">
      <img src="Report/examples/exceptionality/tonemapping/output_no_tone_mapping.png" alt="Figure A" style="width: 100%;">
      <p style="text-align: center;"><b>Figure 1: </b>No tonemapping</p>
    </td>
    <td style="width: 50%; padding: 10px; text-align: center; border: none;">
      <img src="Report/examples/exceptionality/tonemapping/tonemapping_reinhard.png" alt="Figure B" style="width: 100%;">
      <p style="text-align: center;"><b>Figure 2: </b>Reinhard tonemapping</p>
    </td>
  </tr>
  <tr>
    <td style="width: 50%; padding: 10px; text-align: center; border: none;">
      <img src="Report/examples/exceptionality/tonemapping/tonemapping_aces.png" alt="Figure A" style="width: 100%;">
      <p style="text-align: center;"><b>Figure 3: </b>ACES tonemapping</p>
    </td>
    <td style="width: 50%; padding: 10px; text-align: center; border: none;">
      <img src="Report/examples/exceptionality/tonemapping/tonemapping_filmic.png" alt="Figure B" style="width: 100%;">
      <p style="text-align: center;"><b>Figure 4: </b>Filmic tonemapping</p>
    </td>
  </tr>
</table>

This is particularly useful for scenes with more than one light, as it prevents both being scaled relative to their impact on the scene, instead of both being clamped to 1.0. In this way, tonemapping can be used to improve realism. 

<table style="width: 100%; border: none;">
  <tr>
    <td style="width: 50%; padding: 10px; text-align: center; border: none;">
      <img src="Report/examples/exceptionality/tonemapping/output_no_tone_mapping_2_lights.png" alt="Figure A" style="width: 100%;">
      <p style="text-align: center;"><b>Figure 1: </b>No tonemapping</p>
    </td>
    <td style="width: 50%; padding: 10px; text-align: center; border: none;">
      <img src="Report/examples/exceptionality/tonemapping/output_reinhard_2_lights.png" alt="Figure B" style="width: 100%;">
      <p style="text-align: center;"><b>Figure 2: </b>Reinhard tonemapping</p>
    </td>
  </tr>
  <tr>
    <td style="width: 50%; padding: 10px; text-align: center; border: none;">
      <img src="Report/examples/exceptionality/tonemapping/output_aces_2_lights.png" alt="Figure A" style="width: 100%;">
      <p style="text-align: center;"><b>Figure 3: </b>ACES tonemapping</p>
    </td>
    <td style="width: 50%; padding: 10px; text-align: center; border: none;">
      <img src="Report/examples/exceptionality/tonemapping/tonemapping_filmic.png" alt="Figure B" style="width: 100%;">
      <p style="text-align: center;"><b>Figure 4: </b>Filmic tonemapping</p>
    </td>
  </tr>
</table>

# Timeliness

All deadlines were met, with the corresponding features implemented in each module.

## Deviations
### Module 1
This module involved a python exporter which took limited data about objects. For example, module 1 did not require any data about the material of the object and so future versions of the exporter has additional functionality to retrieve material data.

Additionally this module exported 1D scales for the shapes, whereas later versions retrieved values for scale in 3D dimensions. Similarly, the intensity of the light at this point is a 1D vector, whereas in future this was changed to a 3D vector to allow the light to be coloured. The values of this new vector represent the intensity of each colour channel.

Finally, the methods for reading the file from the exporter are in the camera class and header in M1. In future, they are moved to `Code/utilities/scene.cpp` and `Code/utilities/scene.h` for tidiness. 

### Module 2


The intensity vector for the light in this is now a 3D vector to allow coloured lighting. 

<table style="width: 100%; border: none;">
  <tr>
    <td style="width: 50%; padding: 10px; text-align: center; border: none;">
      <img src="Report/examples/M2/scene_test.png" alt="Figure A" style="width: 100%;">
      <p style="text-align: center;"><b>Figure 1: </b>Generated image</p>
    </td>
    <td style="width: 50%; padding: 10px; text-align: center; border: none;">
      <img src="Report/examples/M2/blender_green.png" alt="Figure B" style="width: 100%;">
      <p style="text-align: center;"><b>Figure 2: </b>Original Blender scene</p>
    </td>
  </tr>
</table>

The python exporter also now identifies mesh types by counting the number of polygons, which is a more robust method than relying on the name of the shape.

Additionally, in module 1 the bounds for the cube intersection were +-0.5, but this is changed to +-1.0 after discovering that Blender uses a non-standard system for scaling.

Module 2 also has an additional `Code/utilities/shading.h`, which is responsible for Blinn Phong shading (I was ahead of schedule and so implemented this in module 2 instead of 3). A corresponding material structure, `Code/shapes/material.h`, was added to store the relevant material properties. In module 2, this is just ambient diffuse specular and shininess.

A class was added to represent 4x4 matrices, `Code/utilities/matrix4x4.h` and `Code/utilities/matrix4x4.cpp`, providing utilities for geometric transformations. This was also added to transform rays, as it is computationally more efficient to transform a ray and test it for an intersection against a unit shape than test the original ray against a transformed shape.

A structure was added for the light, `Code/environment/light.h`, necessary for Blinn-Phong. In module 2 this structure simply holds the lights position and intensity.

Classes were added for each of the shapes (circle, plane and, cube in `Code/shapes/`) to allow a different intersection method to be used for each. They inherit from the base class of `Code/shapes/hittable.h` which is contained by `Code/shapes/hittable_list.h` and `Code/shapes/hittable_list.cpp`. This simplifies the raytracer logic for intersection tests, and allows easy iteration through hittable objects.

This module also sees the addition of the BVH acceleration, implemented in `Code/acceleration/bvh.h` and `Code/acceleration/bvh.cpp`. It is built using an AABB (Axis-Aligned Bounding Box) tree,`Code/acceleration/aabb.cpp` and `Code/acceleration/aabb.h`, for computational efficiency.


### Module 3

This module used a sensor size imported from Blender. I discovered that this might not necessarily be the correct aspect ratio, which was causing vertical squashing in my output images. Therefore since the module 3 submission I have updated it to calculate sensor size according to the Blender camera aspect ratio.

### Exceptionality

Tone mapping

# Parameters

| Parameter / Property    | Location / Method                                         | Description                                                                                                                                                                                                                                                                                                    |
|:------------------------|:----------------------------------------------------------|:---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **Global Settings**     |                                                           |                                                                                                                                                                                                                                                                                                                |
| `samples_per_pixel`     | `config.json`                                             | Default number of rays cast per pixel (Antialiasing). Higher is slower but smoother. This can be overridden using the `--aa <int>` flag.                                                                                                                                                                       |
| `max_bounces`           | `config.json`                                             | Maximum recursion depth for reflections/refractions.                                                                                                                                                                                                                                                           |
| `exposure`              | `config.json`                                             | Default brightness multiplier for the final image. This can be overridden using the `--exposure <float>` flag                                                                                                                                                                                                  |
| `shutter_time`          | `config.json`                                             | Default duration the shutter is open (used for motion blur calculations). This can be overridden using the `--motion-blur <float>` flag                                                                                                                                                                        |
| `shadow_samples`        | `config.json`                                             | Number of shadow rays cast per light source per hit (soft shadows). Note that for soft shadows to exist, the light source must have a radius greater than 0.0.                                                                                                                                                 |
| `glossy_samples`        | `config.json`                                             | Number of reflection rays scattered for rough surfaces.                                                                                                                                                                                                                                                        |
| `epsilon`               | `config.json`                                             | Small offset value to prevent self-shadowing acne.                                                                                                                                                                                                                                                             |
| `ray_march_steps`       | `config.json`                                             | Maximum iterations for ray marching complex shapes.                                                                                                                                                                                                                                                            |
| `displacement_strength` | `config.json`                                             | Intensity of displacement mapping on surfaces.                                                                                                                                                                                                                                                                 |
| `background`            | `config.json`                                             | The default R, G, B values of background pixels.                                                                                                                                                                                                                                                               |
| **Command Line Flags**  |                                                           |                                                                                                                                                                                                                                                                                                                |
| `--aa <int>`            | Command Line                                              | Overrides `samples_per_pixel` from config.                                                                                                                                                                                                                                                                     |
| `--exposure <float>`    | Command Line                                              | Overrides `exposure` from config.                                                                                                                                                                                                                                                                              |
| `--motion-blur <float>` | Command Line                                              | Overrides `shutter_time` from config. Enables motion blur.                                                                                                                                                                                                                                                     |
| `--shadows`             | Command Line                                              | Enable shadow calculations (defaults to off).                                                                                                                                                                                                                                                                  |
| `--fresnel`             | Command Line                                              | Enable Fresnel equations for realistic reflection weighting.                                                                                                                                                                                                                                                   |
| `--normals`             | Command Line                                              | Visualise the ray intersections with objects by colouring pixels according to the normals of the hit points.                                                                                                                                                                                                   |
| `--parallel`            | Command Line                                              | Enables multi-threading (OpenMP) for faster rendering. If OpenMP is not available, the program will run with a single thread.                                                                                                                                                                                  |
| `--no-bvh`              | Command Line                                              | Disables the Bounding Volume Hierarchy (acceleration structure).                                                                                                                                                                                                                                               |
| `--time <int>`          | Command Line                                              | Runs the render `<int>` times and logs performance stats.                                                                                                                                                                                                                                                      |
| `--tonemap <string>`    | Command Line                                              | Applies tone mapping. The string can be `reinhard`, `aces`, or `filmic`, corresponding to the tone mapping algorithm used. If this flag is not present, the pixel values will simply be clamped to a range.                                                                                                    |
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