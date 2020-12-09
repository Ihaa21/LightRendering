#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "shader_descriptor_layouts.cpp"
#include "shader_blinn_phong_lighting.cpp"

//
// NOTE: Descriptor Layouts
//

MATERIAL_DESCRIPTOR_LAYOUT(0)
SCENE_DESCRIPTOR_LAYOUT(1)

layout(set = 2, binding = 0) uniform sampler2D GBufferPositionTexture;
layout(set = 2, binding = 1) uniform sampler2D GBufferNormalTexture;
layout(set = 2, binding = 2) uniform sampler2D GBufferColorTexture;

//
// NOTE: GBuffer Vertex
//

#if GBUFFER_VERT

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

//
// NOTE: GBuffer Fragment
//

#if GBUFFER_FRAG

layout(location = 0) in vec3 InViewPos;
layout(location = 1) in vec3 InViewNormal;
layout(location = 2) in vec2 InUv;

layout(location = 0) out vec4 OutViewPos;
layout(location = 1) out vec4 OutViewNormal;
layout(location = 2) out vec4 OutColor;

void main()
{
    OutViewPos = vec4(InViewPos, 0);
    // TODO: Add normal mapping
    OutViewNormal = vec4(normalize(InViewNormal), 0);
    OutColor = texture(ColorTexture, InUv);
}

#endif

//
// NOTE: Point Light Vertex
//

#if POINT_LIGHT_VERT

layout(location = 0) in vec3 InPos;

layout(location = 0) out flat uint InstanceId;

void main()
{
    mat4 Transform = PointLightTransforms[gl_InstanceIndex];
    gl_Position = Transform * vec4(InPos, 1);
    InstanceId = gl_InstanceIndex;
}

#endif

//
// NOTE: Point Light Fragment
//

#if POINT_LIGHT_FRAG

layout(location = 0) in flat uint InInstanceId;

layout(location = 0) out vec4 OutColor;

void main()
{
    vec3 CameraPos = vec3(0, 0, 0);
    vec2 Uv = gl_FragCoord.xy / vec2(textureSize(GBufferPositionTexture, 0));
    
    // TODO: This can all become loads since we are rendering at same resolution
    vec3 SurfacePos = texture(GBufferPositionTexture, Uv).xyz;
    vec3 SurfaceNormal = texture(GBufferNormalTexture, Uv).xyz;
    vec3 SurfaceColor = texture(GBufferColorTexture, Uv).rgb;
    vec3 View = normalize(CameraPos - SurfacePos);

    point_light CurrLight = PointLights[InInstanceId];
    vec3 LightDir = normalize(CurrLight.Pos - SurfacePos);
    OutColor = vec4(BlinnPhongLighting(View, SurfaceColor, SurfaceNormal, 32, LightDir, PointLightAttenuate(SurfacePos, CurrLight)), 1);
}

#endif

//
// NOTE: Directional Light Vert
//

#if DIRECTIONAL_LIGHT_VERT

layout(location = 0) in vec3 InPos;

void main()
{
    gl_Position = 2.0*vec4(InPos, 1);
}

#endif

//
// NOTE: Directional Light Fragment
//

#if DIRECTIONAL_LIGHT_FRAG

layout(location = 0) out vec3 OutColor;

void main()
{
    vec3 CameraPos = vec3(0, 0, 0);
    ivec2 PixelPos = ivec2(gl_FragCoord.xy);
    
    // TODO: This can all become loads since we are rendering at same resolution
    vec3 SurfacePos = texelFetch(GBufferPositionTexture, PixelPos, 0).xyz;
    vec3 SurfaceNormal = texelFetch(GBufferNormalTexture, PixelPos, 0).xyz;
    vec3 SurfaceColor = texelFetch(GBufferColorTexture, PixelPos, 0).rgb;
    vec3 View = normalize(CameraPos - SurfacePos);
    OutColor = BlinnPhongLighting(View, SurfaceColor, SurfaceNormal, 32, DirectionalLight.Dir, DirectionalLight.Color) + DirectionalLight.AmbientLight;
}

#endif
