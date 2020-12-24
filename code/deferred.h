#pragma once

struct deferred_state
{
    vk_linear_arena RenderTargetArena;
    
    // NOTE: GBuffer
    VkImage GBufferPositionImage;
    render_target_entry GBufferPositionEntry;
    VkImage GBufferNormalImage;
    render_target_entry GBufferNormalEntry;
    VkImage GBufferColorImage;
    render_target_entry GBufferColorEntry;
    VkDescriptorSetLayout GBufferDescLayout;
    VkDescriptorSet GBufferDescriptor;

    VkImage OutColorImage;
    render_target_entry OutColorEntry;
    VkImage DepthImage;
    render_target_entry DepthEntry;
    render_target RenderTarget;
    
    vk_pipeline* GBufferPipeline;
    vk_pipeline* PointLightPipeline;
    vk_pipeline* DirectionalLightPipeline;

    render_mesh* QuadMesh;
    render_mesh* SphereMesh;
};
