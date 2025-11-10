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
    explicit Scene(const std::string& scene_filepath, bool build_bvh = true, double exposure = 1.0, bool enable_shadows = false, int glossy_samples = 0, double shutter_time = 0.0);

    // access the loaded camera
    const Camera& getCamera() const { return *m_camera; }
    // access the loaded world (list of shapes)
    const HittableList& getWorld() const { return m_world; }
    // access the loaded lights
    const std::vector<PointLight>& getLights() const { return m_lights; }
    double getExposure() const {return m_exposure; }
    bool shadows_enabled() const {return m_shadows_enabled; }
    int get_glossy_samples() const { return m_glossy_samples;}
    double get_shutter_time() const { return m_shutter_time;}



private:
    void parseSceneFile(const std::string& filepath);

    HittableList m_world; // The list of all shapes
    std::unique_ptr<Camera> m_camera; // camera
    std::vector<PointLight> m_lights; // lights
    double m_exposure = 1.0;
    bool m_shadows_enabled;
    int m_glossy_samples;
    double m_shutter_time;
};

#endif //B216602_SCENE_H