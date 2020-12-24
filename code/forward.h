#pragma once

struct forward_state
{
    vk_linear_arena RenderTargetArena;
    VkImage ColorImage;
    render_target_entry ColorEntry;
    VkImage DepthImage;
    render_target_entry DepthEntry;
    render_target RenderTarget;

    vk_pipeline* Pipeline;
};
