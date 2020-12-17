#pragma once

struct deferred_state
{
    vk_linear_arena RenderTargetArena;
    
    // NOTE: GBuffer
    render_target_entry GBufferPositionEntry;
    render_target_entry GBufferNormalEntry;
    render_target_entry GBufferColorEntry;
    VkDescriptorSetLayout GBufferDescLayout;
    VkDescriptorSet GBufferDescriptor;
    
    render_target_entry OutColorEntry;
    render_target_entry DepthEntry;
    render_target RenderTarget;
    
    vk_pipeline* GBufferPipeline;
    vk_pipeline* PointLightPipeline;
    vk_pipeline* DirectionalLightPipeline;

    render_mesh* QuadMesh;
    render_mesh* SphereMesh;
};
