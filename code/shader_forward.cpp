#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "shader_blinn_phong_lighting.cpp"
#include "shader_descriptor_layouts.cpp"

MATERIAL_DESCRIPTOR_LAYOUT(0)
SCENE_DESCRIPTOR_LAYOUT(1)

#if VERTEX_SHADER

layout(location = 0) in vec3 InPos;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in vec2 InUv;

layout(location = 0) out vec3 OutViewPos;
layout(location = 1) out vec3 OutViewNormal;
layout(location = 2) out vec2 OutUv;

void main()
{
    instance_entry Entry = InstanceBuffer[gl_InstanceIndex];
    
    gl_Position = Entry.WVPTransform * vec4(InPos, 1);
    OutViewPos = (Entry.WVTransform * vec4(InPos, 1)).xyz;
    OutViewNormal = (Entry.WVTransform * vec4(InNormal, 0)).xyz;
    OutUv = InUv;
}

#endif

#if FRAGMENT_SHADER

layout(location = 0) in vec3 InViewPos;
layout(location = 1) in vec3 InViewNormal;
layout(location = 2) in vec2 InUv;

layout(location = 0) out vec4 OutColor;

void main()
{
    vec3 CameraPos = vec3(0, 0, 0);
    
    // TODO: Support alpha
    vec4 TexelColor = texture(ColorTexture, InUv);
    
    vec3 SurfacePos = InViewPos;
    // TODO: Add normal mapping
    vec3 SurfaceNormal = normalize(InViewNormal);
    vec3 SurfaceColor = TexelColor.rgb;
    vec3 View = normalize(CameraPos - SurfacePos);
    vec3 Color = vec3(0);

    // NOTE: Calculate lighting for point lights
    for (int i = 0; i < SceneBuffer.NumPointLights; ++i)
    {
        point_light CurrLight = PointLights[i];
        vec3 LightDir = normalize(CurrLight.Pos - SurfacePos);
        Color += BlinnPhongLighting(View, SurfaceColor, SurfaceNormal, 32, LightDir, PointLightAttenuate(SurfacePos, CurrLight));
    }

    // NOTE: Calculate lighting for directional lights
    {
        Color += BlinnPhongLighting(View, SurfaceColor, SurfaceNormal, 32, DirectionalLight.Dir, DirectionalLight.Color);
        Color += DirectionalLight.AmbientLight;
    }

    OutColor = vec4(Color, 1);
}

#endif
