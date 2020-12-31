#pragma once

#define TILE_SIZE_IN_PIXELS 8
#define MAX_LIGHTS_PER_TILE 1024

struct tiled_forward_globals
{
    // TODO: Move to camera?
    m4 InverseProjection;
    v2 ScreenSize;
    u32 GridSizeX;
    u32 GridSizeY;
};

struct tiled_forward_non_msaa_targets
{
    VkImage ColorImage;
    render_target_entry ColorEntry;
    VkImage DepthImage;
    render_target_entry DepthEntry;
};

struct tiled_forward_msaa_targets
{    
    // NOTE: MSAA Render Targets
    VkImage MsaaColorImage;
    render_target_entry MsaaColorEntry;
    VkImage MsaaDepthImage;
    render_target_entry MsaaDepthEntry;

    // NOTE: Resolved Render Targets
    VkImage ResolvedColorImage;
    render_target_entry ResolvedColorEntry;
    VkImage ResolvedDepthImage;
    render_target_entry ResolvedDepthEntry;
};

struct tiled_forward_state
{
    vk_linear_arena RenderTargetArena;

    b32 IsMsaaEnabled;
    tiled_forward_msaa_targets MsaaTargets;
    tiled_forward_non_msaa_targets NonMsaaTargets;
    render_target DepthPrePass;
    render_target ColorPass;
    
    // NOTE: Resolve Depth Data
    VkDescriptorSet ResolveDepthDescriptor;
    vk_pipeline* ResolveDepthPipeline;

    // NOTE: Global forward data
    VkBuffer TiledForwardGlobals;
    VkBuffer GridFrustums;
    VkBuffer LightIndexList_O;
    VkBuffer LightIndexCounter_O;
    vk_image LightGrid_O;
    VkBuffer LightIndexList_T;
    VkBuffer LightIndexCounter_T;
    vk_image LightGrid_T;
    VkDescriptorSetLayout TiledForwardDescLayout;
    VkDescriptorSet TiledForwardDescriptor;
    
    vk_pipeline* GridFrustumPipeline;
    vk_pipeline* DepthPrePassPipeline;
    vk_pipeline* LightCullPipeline;
    vk_pipeline* TiledForwardPipeline;
};

