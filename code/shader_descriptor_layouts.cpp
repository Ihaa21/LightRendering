
//
// NOTE: Material
//

#define MATERIAL_DESCRIPTOR_LAYOUT(set_number)                          \
    layout(set = set_number, binding = 0) uniform sampler2D ColorTexture; \
    layout(set = set_number, binding = 1) uniform sampler2D NormalTexture; \

//
// NOTE: Scene
//

#include "shader_light_types.cpp"

struct instance_entry
{
    mat4 WVTransform;
    mat4 WVPTransform;
};

#define SCENE_DESCRIPTOR_LAYOUT(set_number)                             \
    layout(set = set_number, binding = 0) uniform scene_buffer          \
    {                                                                   \
        vec3 CameraPos;                                                 \
        uint NumPointLights;                                            \
    } SceneBuffer;                                                      \
                                                                        \
    layout(set = set_number, binding = 1) buffer instance_buffer        \
    {                                                                   \
        instance_entry InstanceBuffer[];                                \
    };                                                                  \
                                                                        \
    layout(set = set_number, binding = 2) buffer point_light_buffer     \
    {                                                                   \
        point_light PointLights[];                                      \
    };                                                                  \
                                                                        \
    layout(set = set_number, binding = 3) buffer directional_light_buffer \
    {                                                                   \
        directional_light DirectionalLight;                             \
    };                                                                  \
                                                                        \
    layout(set = set_number, binding = 4) buffer point_light_transforms \
    {                                                                   \
        mat4 PointLightTransforms[];                                    \
    };                                                                  \
    
