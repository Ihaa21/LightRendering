#pragma once

struct forward_state
{
    render_target_entry ColorEntry;
    render_target_entry DepthEntry;
    render_target RenderTarget;

    vk_pipeline* Pipeline;
};
