#pragma once

struct forward_state
{
    vk_linear_arena RenderTargetArena;
    render_target_entry ColorEntry;
    render_target_entry DepthEntry;
    render_target RenderTarget;

    vk_pipeline* Pipeline;
};
