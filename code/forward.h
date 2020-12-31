#pragma once

struct forward_non_msaa_targets
{
    VkImage ColorImage;
    render_target_entry ColorEntry;
    VkImage DepthImage;
    render_target_entry DepthEntry;
};

struct forward_msaa_targets
{
    VkImage MsaaColorImage;
    render_target_entry MsaaColorEntry;
    VkImage ResolvedColorImage;
    render_target_entry ResolvedColorEntry;
    VkImage MsaaDepthImage;
    render_target_entry MsaaDepthEntry;
};

struct forward_state
{
    vk_linear_arena RenderTargetArena;

    b32 IsMsaaEnabled;
    forward_msaa_targets MsaaTargets;
    forward_non_msaa_targets NonMsaaTargets;
    render_target RenderTarget;

    vk_pipeline* Pipeline;
};
