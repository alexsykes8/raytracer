#ifndef B216602_SCENE_H
#define B216602_SCENE_H

#include <string>
#include <memory>
#include "shapes/hittable_list.h"
#include "camera.h"
#include "light.h"
#include "matrix4x4.h"

class Scene {
public:
    // load scene from file
    explicit Scene(const std::string& scene_filepath, bool build_bvh = true, double exposure = 1.0, bool enable_shadows = false);
    // access the loaded camera
    const Camera& getCamera() const { return *m_camera; }
    // access the loaded world (list of shapes)
    const HittableList& getWorld() const { return m_world; }
    // access the loaded lights
    const std::vector<PointLight>& getLights() const { return m_lights; }

    double getExposure() const { return m_exposure; }
    bool getEnableShadows() const { return m_enable_shadows; }



private:
    void parseSceneFile(const std::string& filepath);

    HittableList m_world; // The list of all shapes
    std::unique_ptr<Camera> m_camera; // camera
    std::vector<PointLight> m_lights; // lights
    double m_exposure;
    bool m_enable_shadows;
};

#endif //B216602_SCENE_H