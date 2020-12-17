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

struct tiled_forward_state
{
    vk_linear_arena RenderTargetArena;

    render_target_entry ColorEntry;
    render_target_entry DepthEntry;
    render_target DepthPrePass;
    render_target ColorPass;

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

