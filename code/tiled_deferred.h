#pragma once

#define TILE_SIZE_IN_PIXELS 8
#define MAX_LIGHTS_PER_TILE 1024

struct tiled_deferred_globals
{
    // TODO: Move to camera?
    m4 InverseProjection;
    v2 ScreenSize;
    u32 GridSizeX;
    u32 GridSizeY;
};

struct tiled_deferred_state
{
    vk_linear_arena RenderTargetArena;
    
    // NOTE: GBuffer
    render_target_entry GBufferPositionEntry;
    render_target_entry GBufferNormalEntry;
    render_target_entry GBufferColorEntry;
    render_target_entry DepthEntry;
    render_target_entry OutColorEntry;
    render_target GBufferPass;
    render_target LightingPass;

    // NOTE: Global data
    VkBuffer TiledDeferredGlobals;
    VkBuffer GridFrustums;
    VkBuffer LightIndexList_O;
    VkBuffer LightIndexCounter_O;
    vk_image LightGrid_O;
    VkBuffer LightIndexList_T;
    VkBuffer LightIndexCounter_T;
    vk_image LightGrid_T;
    VkDescriptorSetLayout TiledDeferredDescLayout;
    VkDescriptorSet TiledDeferredDescriptor;

    render_mesh* QuadMesh;
    
    vk_pipeline* GridFrustumPipeline;
    vk_pipeline* GBufferPipeline;
    vk_pipeline* LightCullPipeline;
    vk_pipeline* LightingPipeline;
};

