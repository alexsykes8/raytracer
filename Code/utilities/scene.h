#ifndef B216602_SCENE_H
#define B216602_SCENE_H

#include <string>
#include <memory>
#include "../shapes/hittable_list.h"
#include "../environment/camera.h"
#include "../environment/light.h"
#include "matrix4x4.h"
#include "../environment/HDRImage.h"


class Scene {
public:
    // load scene from file
    explicit Scene(const std::string& scene_filepath, bool build_bvh = true, double exposure = 1.0, bool enable_shadows = false, int glossy_samples = 0, double shutter_time = 0.0, bool enable_fresnel = false, bool render_normals = false);
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
    bool fresnel_enabled() const { return m_fresnel_enabled; }
    bool rendering_normals() const { return m_render_normals; }

    bool has_hdr_background() const { return m_hdr_background != nullptr; }
    const HDRImage* get_hdr_background() const { return m_hdr_background.get(); }
    int get_shadow_samples() const { return m_shadow_samples; }
    double get_epsilon() const { return m_epsilon; }
    int get_max_bounces() const { return m_max_bounces; }



private:
    void parseSceneFile(const std::string& filepath);

    HittableList m_world; // The list of all shapes
    std::unique_ptr<Camera> m_camera; // camera
    std::vector<PointLight> m_lights; // lights
    double m_exposure = 1.0;
    bool m_shadows_enabled;
    int m_glossy_samples;
    double m_shutter_time;
    bool m_fresnel_enabled;
    bool m_render_normals;
    int m_shadow_samples;
    double m_epsilon;
    int m_max_bounces;



    std::shared_ptr<HDRImage> m_hdr_background;
};

#endif //B216602_SCENE_H