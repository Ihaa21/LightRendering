#pragma once

#include "math\math.h"

//
// NOTE: Memory
//

struct vk_gpu_ptr
{
    VkDeviceMemory* Memory;
    u64 Offset;
};

struct vk_gpu_linear_arena
{
    u64 Size;
    u64 Used;
    VkDeviceMemory Memory;
};

struct vk_gpu_temp_mem
{
    vk_gpu_linear_arena* Arena;
    u64 Used;
};

//
// NOTE: Descriptor Layout Builder
//

// TODO: Dynamic Arena
struct vk_descriptor_layout_builder
{
    u32 CurrNumBindings;
    VkDescriptorSetLayoutBinding Bindings[100];
    VkDescriptorSetLayout* Layout;
};

//
// NOTE: Render Pass Builder
//

struct vk_render_pass_builder
{
    temp_mem TempMem;
    linear_arena* Arena;

    // NOTE: Attachment Data
    u32 MaxNumAttachments;
    u32 NumAttachments;
    VkAttachmentDescription* Attachments;

    // NOTE: Dependencies
    u32 MaxNumDependencies;
    u32 NumDependencies;
    VkSubpassDependency* Dependencies;
    
    // NOTE: SubPass data
    u32 MaxNumInputAttachmentRefs;
    u32 NumInputAttachmentRefs;
    VkAttachmentReference* InputAttachmentRefs;

    u32 MaxNumColorAttachmentRefs;
    u32 NumColorAttachmentRefs;
    VkAttachmentReference* ColorAttachmentRefs;

    u32 MaxNumDepthAttachmentRefs;
    u32 NumDepthAttachmentRefs;
    VkAttachmentReference* DepthAttachmentRefs;
    
    u32 MaxNumSubPasses;
    u32 NumSubPasses;
    VkSubpassDescription* SubPasses;
    
};

//
// NOTE: Barrier Manager
//

struct barrier_mask
{
    VkAccessFlags AccessMask;
    VkPipelineStageFlags StageMask;
};

struct vk_barrier_manager
{
    u32 MaxNumMemoryBarriers;
    u32 NumMemoryBarriers;
    VkMemoryBarrier* MemoryBarrierArray;

    u32 MaxNumImageBarriers;
    u32 NumImageBarriers;
    VkImageMemoryBarrier* ImageBarrierArray;

    u32 MaxNumBufferBarriers;
    u32 NumBufferBarriers;
    VkBufferMemoryBarrier* BufferBarrierArray;

    VkPipelineStageFlags SrcStageFlags;
    VkPipelineStageFlags DstStageFlags;
};

//
// NOTE: Descriptor Updater
//

struct vk_descriptor_manager
{
    linear_arena Arena;
    
    u32 MaxNumWrites;
    u32 NumWrites;
    VkWriteDescriptorSet* WriteArray;
};

//
// NOTE: Pipeline Manager
//

struct vk_pipeline
{
    VkPipeline Handle;
    VkPipelineLayout Layout;
};

struct vk_shader_ref
{
    char* FileName;
    char* MainName;
    FILETIME ModifiedTime;
    VkShaderStageFlagBits Stage;
};

enum vk_pipeline_entry_type
{
    VkPipelineEntry_None,
    
    VkPipelineEntry_Graphics,
    VkPipelineEntry_Compute,
};

struct vk_pipeline_graphics_entry
{
    VkVertexInputBindingDescription* VertBindings;
    VkVertexInputAttributeDescription* VertAttributes;
    VkPipelineVertexInputStateCreateInfo VertexInputState;
    
    VkPipelineInputAssemblyStateCreateInfo InputAssemblyState;
    VkPipelineTessellationStateCreateInfo TessellationState;

    VkViewport* ViewPorts;
    VkRect2D* Scissors;
    VkPipelineViewportStateCreateInfo ViewportState;
    
    VkPipelineRasterizationStateCreateInfo RasterizationState;
    VkPipelineMultisampleStateCreateInfo MultisampleState;    
    VkPipelineDepthStencilStateCreateInfo DepthStencilState;

    VkPipelineColorBlendAttachmentState* Attachments;
    VkPipelineColorBlendStateCreateInfo ColorBlendState;

    VkDynamicState* DynamicStates;
    VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo;
    
    VkGraphicsPipelineCreateInfo PipelineCreateInfo;
};

struct vk_pipeline_compute_entry
{
    VkComputePipelineCreateInfo PipelineCreateInfo;
};

// TODO: Verify we can only have up to 5 shaders for graphics pipelines
#define VK_MAX_PIPELINE_STAGES 5
struct vk_pipeline_entry
{
    vk_pipeline_entry_type Type;
    union
    {
        vk_pipeline_graphics_entry GraphicsEntry;
        vk_pipeline_compute_entry ComputeEntry;
    };

    u32 NumShaders;
    vk_shader_ref ShaderRefs[VK_MAX_PIPELINE_STAGES];

    vk_pipeline Pipeline;
};

struct vk_pipeline_manager
{
    linear_arena Arena;
    
    u32 MaxNumPipelines;
    u32 NumPipelines;
    vk_pipeline_entry* PipelineArray;
};

//
// NOTE: Pipline Builder
//

enum vk_pipeline_builder_flags
{
    VkPipelineFlag_HasDepthStencil = 1 << 0,
};

struct vk_pipeline_builder_shader
{
    char* FileName;
    char* MainName;
    VkShaderStageFlagBits Stage;
};

struct vk_pipeline_builder
{
    linear_arena* Arena;
    temp_mem TempMem;

    u32 Flags;

    // NOTE: Shader data
    u32 NumShaders;
    vk_pipeline_builder_shader Shaders[VK_MAX_PIPELINE_STAGES];
    
    // NOTE: Vertex data
    u32 CurrVertexBindingSize;
    u32 CurrVertexLocation;

    u32 MaxNumVertexBindings;
    u32 NumVertexBindings;
    VkVertexInputBindingDescription* VertexBindings;

    u32 MaxNumVertexAttributes;
    u32 NumVertexAttributes;
    VkVertexInputAttributeDescription* VertexAttributes;

    // NOTE: Input Assembly Data
    VkPipelineInputAssemblyStateCreateInfo InputAssembly;
    
    // NOTE: Depth Stencil Data
    VkPipelineDepthStencilStateCreateInfo DepthStencil;

    // NOTE: Color Attachment Data
    u32 MaxNumColorAttachments;
    u32 NumColorAttachments;
    VkPipelineColorBlendAttachmentState* ColorAttachments;

    VkPipelineRasterizationStateCreateInfo RasterizationState;
    
    VkGraphicsPipelineCreateInfo PipelineCreateInfo;
};

inline void VkPipelineInputAssemblyAdd(vk_pipeline_builder* Builder, VkPrimitiveTopology Topology, VkBool32 PrimRestart);

//
// NOTE: Transfer updater
//

struct vk_buffer_transfer
{
    VkBuffer Buffer;
    u64 DstOffset;
    u64 Size;
    u64 StagingOffset;

    barrier_mask InputMask;
    barrier_mask OutputMask;
};

struct vk_image_transfer
{
    u64 StagingOffset;

    VkImageAspectFlags AspectMask;
    VkImage Image;
    u32 Width;
    u32 Height;
    
    barrier_mask InputMask;
    VkImageLayout InputLayout;

    barrier_mask OutputMask;
    VkImageLayout OutputLayout;
};

// TODO: Add resource reading 
struct vk_transfer_manager
{
    linear_arena Arena;

    // NOTE: Vk Constants
    u64 FlushAlignment;
    
    // NOTE: Staging data
    u64 StagingSize;
    u64 StagingOffset;
    u8* StagingPtr;
    VkDeviceMemory StagingMem;
    VkBuffer StagingBuffer;

    // NOTE: Buffer data
    u32 MaxNumBufferTransfers;
    u32 NumBufferTransfers;
    vk_buffer_transfer* BufferTransferArray;
    
    // NOTE: Image data
    u32 MaxNumImageTransfers;
    u32 NumImageTransfers;
    vk_image_transfer* ImageTransferArray;
};

//
// NOTE: Helper structs
//

struct vk_commands
{
    VkCommandBuffer Buffer;
    VkFence Fence;
};

struct vk_image
{
    VkImage Image;
    VkImageView View;
};

#include "vulkan_utils.cpp"
