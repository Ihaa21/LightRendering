
/*

   TODO: Light List Idea

     - Can we reuse the light list from prev frame? Just recheck all the lights in the tiles they are currently in and update that way?
       I guess its not beneficial with just that because you still have to check all tiles in the map incase anything new got in there
  
*/

inline void TiledDeferredSwapChainChange(tiled_deferred_state* State, u32 Width, u32 Height, VkFormat ColorFormat,
                                         render_scene* Scene, VkDescriptorSet* OutputRtSet)
{
    b32 ReCreate = State->RenderTargetArena.Used != 0;
    VkArenaClear(&State->RenderTargetArena);
    
    // NOTE: Render Target Data
    {
        RenderTargetEntryReCreate(&State->RenderTargetArena, Width, Height, VK_FORMAT_R32G32B32A32_SFLOAT,
                                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                  VK_IMAGE_ASPECT_COLOR_BIT, &State->GBufferPositionImage, &State->GBufferPositionEntry);
        RenderTargetEntryReCreate(&State->RenderTargetArena, Width, Height, VK_FORMAT_R32G32B32A32_SFLOAT,
                                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                  VK_IMAGE_ASPECT_COLOR_BIT, &State->GBufferNormalImage, &State->GBufferNormalEntry);
        RenderTargetEntryReCreate(&State->RenderTargetArena, Width, Height, VK_FORMAT_R32G32B32A32_SFLOAT,
                                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                  VK_IMAGE_ASPECT_COLOR_BIT, &State->GBufferColorImage, &State->GBufferColorEntry);
        RenderTargetEntryReCreate(&State->RenderTargetArena, Width, Height, VK_FORMAT_D32_SFLOAT,
                                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                  VK_IMAGE_ASPECT_DEPTH_BIT, &State->DepthImage, &State->DepthEntry);
        RenderTargetEntryReCreate(&State->RenderTargetArena, Width, Height, ColorFormat,
                                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                  VK_IMAGE_ASPECT_COLOR_BIT, &State->OutColorImage, &State->OutColorEntry);

        if (ReCreate)
        {
            RenderTargetUpdateEntries(&DemoState->TempArena, &State->GBufferPass);
            RenderTargetUpdateEntries(&DemoState->TempArena, &State->LightingPass);
        }
        
        VkDescriptorImageWrite(&RenderState->DescriptorManager, *OutputRtSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                               State->OutColorEntry.View, DemoState->LinearSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        
        // NOTE: GBuffer
        VkDescriptorImageWrite(&RenderState->DescriptorManager, State->TiledDeferredDescriptor, 8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                               State->GBufferPositionEntry.View, DemoState->PointSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        VkDescriptorImageWrite(&RenderState->DescriptorManager, State->TiledDeferredDescriptor, 9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                               State->GBufferNormalEntry.View, DemoState->PointSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        VkDescriptorImageWrite(&RenderState->DescriptorManager, State->TiledDeferredDescriptor, 10, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                               State->GBufferColorEntry.View, DemoState->PointSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        VkDescriptorImageWrite(&RenderState->DescriptorManager, State->TiledDeferredDescriptor, 11, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                               State->DepthEntry.View, DemoState->PointSampler, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
    }
    
    // NOTE: Tiled Data
    {
        u32 NumTilesX = CeilU32(f32(Width) / f32(TILE_SIZE_IN_PIXELS));
        u32 NumTilesY = CeilU32(f32(Height) / f32(TILE_SIZE_IN_PIXELS));

        // NOTE: Destroy old data
        if (ReCreate)
        {
            vkDestroyBuffer(RenderState->Device, State->GridFrustums, 0);
            vkDestroyBuffer(RenderState->Device, State->LightIndexList_O, 0);
            vkDestroyBuffer(RenderState->Device, State->LightIndexList_T, 0);
            vkDestroyImageView(RenderState->Device, State->LightGrid_O.View, 0);
            vkDestroyImage(RenderState->Device, State->LightGrid_O.Image, 0);
            vkDestroyImageView(RenderState->Device, State->LightGrid_T.View, 0);
            vkDestroyImage(RenderState->Device, State->LightGrid_T.Image, 0);
        }
        
        State->GridFrustums = VkBufferCreate(RenderState->Device, &State->RenderTargetArena, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                             sizeof(frustum) * NumTilesX * NumTilesY);
        State->LightGrid_O = VkImageCreate(RenderState->Device, &State->RenderTargetArena, NumTilesX, NumTilesY, VK_FORMAT_R32G32_UINT,
                                           VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        State->LightIndexList_O = VkBufferCreate(RenderState->Device, &State->RenderTargetArena, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                 sizeof(u32) * MAX_LIGHTS_PER_TILE * NumTilesX * NumTilesY);
        State->LightGrid_T = VkImageCreate(RenderState->Device, &State->RenderTargetArena, NumTilesX, NumTilesY, VK_FORMAT_R32G32_UINT,
                                           VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        State->LightIndexList_T = VkBufferCreate(RenderState->Device, &State->RenderTargetArena, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                 sizeof(u32) * MAX_LIGHTS_PER_TILE * NumTilesX * NumTilesY);

        VkDescriptorBufferWrite(&RenderState->DescriptorManager, State->TiledDeferredDescriptor, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, State->GridFrustums);
        VkDescriptorImageWrite(&RenderState->DescriptorManager, State->TiledDeferredDescriptor, 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                               State->LightGrid_O.View, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL);
        VkDescriptorBufferWrite(&RenderState->DescriptorManager, State->TiledDeferredDescriptor, 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, State->LightIndexList_O);
        VkDescriptorImageWrite(&RenderState->DescriptorManager, State->TiledDeferredDescriptor, 5, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                               State->LightGrid_T.View, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL);
        VkDescriptorBufferWrite(&RenderState->DescriptorManager, State->TiledDeferredDescriptor, 6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, State->LightIndexList_T);
    }

    VkDescriptorManagerFlush(RenderState->Device, &RenderState->DescriptorManager);
    
    // NOTE: Init Grid Frustums
    vk_commands Commands = RenderState->Commands;
    VkCommandsBegin(RenderState->Device, Commands);
    {
        // NOTE: Init our images
        VkBarrierImageAdd(&RenderState->BarrierManager, VK_ACCESS_MEMORY_READ_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_MEMORY_READ_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_IMAGE_LAYOUT_GENERAL,
                          VK_IMAGE_ASPECT_COLOR_BIT, State->LightGrid_O.Image);
        VkBarrierImageAdd(&RenderState->BarrierManager, VK_ACCESS_MEMORY_READ_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_MEMORY_READ_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_IMAGE_LAYOUT_GENERAL,
                          VK_IMAGE_ASPECT_COLOR_BIT, State->LightGrid_T.Image);
        VkBarrierManagerFlush(&RenderState->BarrierManager, Commands.Buffer);

        // NOTE: Update our tiled deferred globals
        {
            tiled_deferred_globals* Data = VkTransferPushWriteStruct(&RenderState->TransferManager, State->TiledDeferredGlobals, tiled_deferred_globals,
                                                                     BarrierMask(VkAccessFlagBits(0), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT),
                                                                     BarrierMask(VK_ACCESS_UNIFORM_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
            *Data = {};
            Data->InverseProjection = Inverse(CameraGetP(&Scene->Camera));
            Data->ScreenSize = V2(RenderState->WindowWidth, RenderState->WindowHeight);
            Data->GridSizeX = CeilU32(f32(RenderState->WindowWidth) / f32(TILE_SIZE_IN_PIXELS));
            Data->GridSizeY = CeilU32(f32(RenderState->WindowHeight) / f32(TILE_SIZE_IN_PIXELS));
        }
        VkTransferManagerFlush(&RenderState->TransferManager, RenderState->Device, RenderState->Commands.Buffer, &RenderState->BarrierManager);

        vkCmdBindPipeline(Commands.Buffer, VK_PIPELINE_BIND_POINT_COMPUTE, State->GridFrustumPipeline->Handle);
        VkDescriptorSet DescriptorSets[] =
            {
                State->TiledDeferredDescriptor,
            };
        vkCmdBindDescriptorSets(Commands.Buffer, VK_PIPELINE_BIND_POINT_COMPUTE, State->GridFrustumPipeline->Layout, 0,
                                ArrayCount(DescriptorSets), DescriptorSets, 0, 0);
        u32 DispatchX = CeilU32(f32(RenderState->WindowWidth) / f32(8 * TILE_SIZE_IN_PIXELS));
        u32 DispatchY = CeilU32(f32(RenderState->WindowHeight) / f32(8 * TILE_SIZE_IN_PIXELS));
        vkCmdDispatch(Commands.Buffer, DispatchX, DispatchY, 1);
    }
    VkCommandsSubmit(RenderState->GraphicsQueue, Commands);
}

inline void TiledDeferredCreate(renderer_create_info CreateInfo, VkDescriptorSet* OutputRtSet, tiled_deferred_state* Result)
{
    *Result = {};

    u64 HeapSize = GigaBytes(1);
    Result->RenderTargetArena = VkLinearArenaCreate(VkMemoryAllocate(RenderState->Device, RenderState->LocalMemoryId, HeapSize), HeapSize);
    
    // NOTE: Create globals
    {        
        Result->TiledDeferredGlobals = VkBufferCreate(RenderState->Device, &RenderState->GpuArena, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                      sizeof(tiled_deferred_globals));
        Result->LightIndexCounter_O = VkBufferCreate(RenderState->Device, &RenderState->GpuArena, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                     sizeof(u32));
        Result->LightIndexCounter_T = VkBufferCreate(RenderState->Device, &RenderState->GpuArena, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                     sizeof(u32));
        
        {
            vk_descriptor_layout_builder Builder = VkDescriptorLayoutBegin(&Result->TiledDeferredDescLayout);

            // NOTE: Tiled Descriptors
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

            // NOTE: GBuffer Descriptors
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            
            VkDescriptorLayoutEnd(RenderState->Device, &Builder);
        }

        Result->TiledDeferredDescriptor = VkDescriptorSetAllocate(RenderState->Device, RenderState->DescriptorPool, Result->TiledDeferredDescLayout);

        // NOTE: Tiled Data
        VkDescriptorBufferWrite(&RenderState->DescriptorManager, Result->TiledDeferredDescriptor, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Result->TiledDeferredGlobals);
        VkDescriptorBufferWrite(&RenderState->DescriptorManager, Result->TiledDeferredDescriptor, 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Result->LightIndexCounter_O);
        VkDescriptorBufferWrite(&RenderState->DescriptorManager, Result->TiledDeferredDescriptor, 7, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Result->LightIndexCounter_T);
    }

    // NOTE: Grid Frustum
    {
        VkDescriptorSetLayout Layouts[] =
            {
                Result->TiledDeferredDescLayout,
            };
            
        Result->GridFrustumPipeline = VkPipelineComputeCreate(RenderState->Device, &RenderState->PipelineManager, &DemoState->TempArena,
                                                              "shader_tiled_deferred_grid_frustum.spv", "main", Layouts, ArrayCount(Layouts));
    }

    TiledDeferredSwapChainChange(Result, CreateInfo.Width, CreateInfo.Height, CreateInfo.ColorFormat, CreateInfo.Scene, OutputRtSet);

    // NOTE: Create PSOs
    // IMPORTANT: We don't do this in a single render pass since we cannot do compute between graphics
    {
        // NOTE: GBuffer Pass
        {
            // NOTE: RT
            {
                render_target_builder Builder = RenderTargetBuilderBegin(&DemoState->Arena, &DemoState->TempArena, CreateInfo.Width, CreateInfo.Height);
                RenderTargetAddTarget(&Builder, &Result->GBufferPositionEntry, VkClearColorCreate(0, 0, 0, 1));
                RenderTargetAddTarget(&Builder, &Result->GBufferNormalEntry, VkClearColorCreate(0, 0, 0, 1));
                RenderTargetAddTarget(&Builder, &Result->GBufferColorEntry, VkClearColorCreate(0, 0, 0, 1));
                RenderTargetAddTarget(&Builder, &Result->DepthEntry, VkClearDepthStencilCreate(0, 0));
                            
                vk_render_pass_builder RpBuilder = VkRenderPassBuilderBegin(&DemoState->TempArena);

                u32 GBufferPositionId = VkRenderPassAttachmentAdd(&RpBuilder, Result->GBufferPositionEntry.Format, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                                  VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED,
                                                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                u32 GBufferNormalId = VkRenderPassAttachmentAdd(&RpBuilder, Result->GBufferNormalEntry.Format, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                                VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED,
                                                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                u32 GBufferColorId = VkRenderPassAttachmentAdd(&RpBuilder, Result->GBufferColorEntry.Format, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                               VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED,
                                                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                u32 DepthId = VkRenderPassAttachmentAdd(&RpBuilder, Result->DepthEntry.Format, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                        VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED,
                                                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

                VkRenderPassSubPassBegin(&RpBuilder, VK_PIPELINE_BIND_POINT_GRAPHICS);
                VkRenderPassColorRefAdd(&RpBuilder, GBufferPositionId, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                VkRenderPassColorRefAdd(&RpBuilder, GBufferNormalId, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                VkRenderPassColorRefAdd(&RpBuilder, GBufferColorId, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                VkRenderPassDepthRefAdd(&RpBuilder, DepthId, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
                VkRenderPassSubPassEnd(&RpBuilder);

                VkRenderPassDependency(&RpBuilder, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                       VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                       VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);

                Result->GBufferPass = RenderTargetBuilderEnd(&Builder, VkRenderPassBuilderEnd(&RpBuilder, RenderState->Device));
            }

            {
                vk_pipeline_builder Builder = VkPipelineBuilderBegin(&DemoState->TempArena);

                // NOTE: Shaders
                VkPipelineShaderAdd(&Builder, "shader_tiled_deferred_gbuffer_vert.spv", "main", VK_SHADER_STAGE_VERTEX_BIT);
                VkPipelineShaderAdd(&Builder, "shader_tiled_deferred_gbuffer_frag.spv", "main", VK_SHADER_STAGE_FRAGMENT_BIT);
                
                // NOTE: Specify input vertex data format
                VkPipelineVertexBindingBegin(&Builder);
                VkPipelineVertexAttributeAdd(&Builder, VK_FORMAT_R32G32B32_SFLOAT, sizeof(v3));
                VkPipelineVertexAttributeAdd(&Builder, VK_FORMAT_R32G32B32_SFLOAT, sizeof(v3));
                VkPipelineVertexAttributeAdd(&Builder, VK_FORMAT_R32G32_SFLOAT, sizeof(v2));
                VkPipelineVertexBindingEnd(&Builder);

                VkPipelineInputAssemblyAdd(&Builder, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
                VkPipelineDepthStateAdd(&Builder, VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER);

                VkPipelineColorAttachmentAdd(&Builder, VK_FALSE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO,
                                             VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);
                VkPipelineColorAttachmentAdd(&Builder, VK_FALSE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO,
                                             VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);
                VkPipelineColorAttachmentAdd(&Builder, VK_FALSE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO,
                                             VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);

                VkDescriptorSetLayout DescriptorLayouts[] =
                    {
                        Result->TiledDeferredDescLayout,
                        CreateInfo.SceneDescLayout,
                        CreateInfo.MaterialDescLayout,
                    };
            
                Result->GBufferPipeline = VkPipelineBuilderEnd(&Builder, RenderState->Device, &RenderState->PipelineManager,
                                                               Result->GBufferPass.RenderPass, 0, DescriptorLayouts, ArrayCount(DescriptorLayouts));

            }
        }
        
        // NOTE: Light Cull
        {
            VkDescriptorSetLayout Layouts[] =
                {
                    Result->TiledDeferredDescLayout,
                    CreateInfo.SceneDescLayout,
                };
            
            Result->LightCullPipeline = VkPipelineComputeCreate(RenderState->Device, &RenderState->PipelineManager, &DemoState->TempArena,
                                                                "shader_tiled_deferred_light_culling.spv", "main", Layouts, ArrayCount(Layouts));
        }

        // NOTE: Lighting Pass 
        {
            // NOTE: RT
            {
                render_target_builder Builder = RenderTargetBuilderBegin(&DemoState->Arena, &DemoState->TempArena, CreateInfo.Width, CreateInfo.Height);
                RenderTargetAddTarget(&Builder, &Result->OutColorEntry, VkClearColorCreate(0, 0, 0, 1));
                            
                vk_render_pass_builder RpBuilder = VkRenderPassBuilderBegin(&DemoState->TempArena);

                u32 OutColorId = VkRenderPassAttachmentAdd(&RpBuilder, Result->OutColorEntry.Format, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                           VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED,
                                                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                VkRenderPassDependency(&RpBuilder, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                       VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);

                VkRenderPassSubPassBegin(&RpBuilder, VK_PIPELINE_BIND_POINT_GRAPHICS);
                VkRenderPassColorRefAdd(&RpBuilder, OutColorId, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                VkRenderPassSubPassEnd(&RpBuilder);

                Result->LightingPass = RenderTargetBuilderEnd(&Builder, VkRenderPassBuilderEnd(&RpBuilder, RenderState->Device));
            }

            {
                vk_pipeline_builder Builder = VkPipelineBuilderBegin(&DemoState->TempArena);

                // NOTE: Shaders
                VkPipelineShaderAdd(&Builder, "shader_tiled_deferred_lighting_vert.spv", "main", VK_SHADER_STAGE_VERTEX_BIT);
                VkPipelineShaderAdd(&Builder, "shader_tiled_deferred_lighting_frag.spv", "main", VK_SHADER_STAGE_FRAGMENT_BIT);
                
                // NOTE: Specify input vertex data format
                VkPipelineVertexBindingBegin(&Builder);
                VkPipelineVertexAttributeAdd(&Builder, VK_FORMAT_R32G32B32_SFLOAT, 2*sizeof(v3) + sizeof(v2));
                VkPipelineVertexBindingEnd(&Builder);

                VkPipelineInputAssemblyAdd(&Builder, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
                VkPipelineColorAttachmentAdd(&Builder, VK_FALSE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO,
                                             VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);

                VkDescriptorSetLayout DescriptorLayouts[] =
                    {
                        Result->TiledDeferredDescLayout,
                        CreateInfo.SceneDescLayout,
                    };
            
                Result->LightingPipeline = VkPipelineBuilderEnd(&Builder, RenderState->Device, &RenderState->PipelineManager,
                                                                Result->LightingPass.RenderPass, 0, DescriptorLayouts, ArrayCount(DescriptorLayouts));
            }
        }
    }
}

inline void TiledDeferredAddMeshes(tiled_deferred_state* State, render_mesh* QuadMesh)
{
    State->QuadMesh = QuadMesh;
}

inline void TiledDeferredRender(vk_commands Commands, tiled_deferred_state* State, render_scene* Scene)
{
    // NOTE: Clear images
    {
        // NOTE: Clear buffers and upload data
        VkClearValue ClearColor = VkClearColorCreate(0, 0, 0, 0);
        VkImageSubresourceRange Range = {};
        Range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        Range.baseMipLevel = 0;
        Range.levelCount = 1;
        Range.baseArrayLayer = 0;
        Range.layerCount = 1;
        
        vkCmdClearColorImage(Commands.Buffer, State->LightGrid_O.Image, VK_IMAGE_LAYOUT_GENERAL, &ClearColor.color, 1, &Range);
        vkCmdClearColorImage(Commands.Buffer, State->LightGrid_T.Image, VK_IMAGE_LAYOUT_GENERAL, &ClearColor.color, 1, &Range);
        vkCmdFillBuffer(Commands.Buffer, State->LightIndexCounter_O, 0, sizeof(u32), 0);
        vkCmdFillBuffer(Commands.Buffer, State->LightIndexCounter_T, 0, sizeof(u32), 0);
    }
    
    // NOTE: GBuffer Pass
    RenderTargetPassBegin(&State->GBufferPass, Commands, RenderTargetRenderPass_SetViewPort | RenderTargetRenderPass_SetScissor);
    {
        vkCmdBindPipeline(Commands.Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, State->GBufferPipeline->Handle);
        {
            VkDescriptorSet DescriptorSets[] =
                {
                    State->TiledDeferredDescriptor,
                    Scene->SceneDescriptor,
                };
            vkCmdBindDescriptorSets(Commands.Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, State->GBufferPipeline->Layout, 0,
                                    ArrayCount(DescriptorSets), DescriptorSets, 0, 0);
        }

        for (u32 InstanceId = 0; InstanceId < Scene->NumOpaqueInstances; ++InstanceId)
        {
            instance_entry* CurrInstance = Scene->OpaqueInstances + InstanceId;
            render_mesh* CurrMesh = Scene->RenderMeshes + CurrInstance->MeshId;

            {
                VkDescriptorSet DescriptorSets[] =
                    {
                        CurrMesh->MaterialDescriptor,
                    };
                vkCmdBindDescriptorSets(Commands.Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, State->GBufferPipeline->Layout, 2,
                                        ArrayCount(DescriptorSets), DescriptorSets, 0, 0);
            }
            
            VkDeviceSize Offset = 0;
            vkCmdBindVertexBuffers(Commands.Buffer, 0, 1, &CurrMesh->VertexBuffer, &Offset);
            vkCmdBindIndexBuffer(Commands.Buffer, CurrMesh->IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(Commands.Buffer, CurrMesh->NumIndices, 1, 0, 0, InstanceId);
        }
    }
    RenderTargetPassEnd(Commands);
    
    // NOTE: Light Culling Pass
    {
        vkCmdBindPipeline(Commands.Buffer, VK_PIPELINE_BIND_POINT_COMPUTE, State->LightCullPipeline->Handle);
        VkDescriptorSet DescriptorSets[] =
            {
                State->TiledDeferredDescriptor,
                Scene->SceneDescriptor,
            };
        vkCmdBindDescriptorSets(Commands.Buffer, VK_PIPELINE_BIND_POINT_COMPUTE, State->LightCullPipeline->Layout, 0,
                                ArrayCount(DescriptorSets), DescriptorSets, 0, 0);
        u32 DispatchX = CeilU32(f32(RenderState->WindowWidth) / f32(TILE_SIZE_IN_PIXELS));
        u32 DispatchY = CeilU32(f32(RenderState->WindowHeight) / f32(TILE_SIZE_IN_PIXELS));
        vkCmdDispatch(Commands.Buffer, DispatchX, DispatchY, 1);
    }

    vkCmdPipelineBarrier(Commands.Buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 0, 0);
    
    // NOTE: Lighting Pass
    RenderTargetPassBegin(&State->LightingPass, Commands, RenderTargetRenderPass_SetViewPort | RenderTargetRenderPass_SetScissor);
    {
        vkCmdBindPipeline(Commands.Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, State->LightingPipeline->Handle);
        {
            VkDescriptorSet DescriptorSets[] =
                {
                    State->TiledDeferredDescriptor,
                    Scene->SceneDescriptor,
                };
            vkCmdBindDescriptorSets(Commands.Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, State->LightingPipeline->Layout, 0,
                                    ArrayCount(DescriptorSets), DescriptorSets, 0, 0);
        }

        VkDeviceSize Offset = 0;
        vkCmdBindVertexBuffers(Commands.Buffer, 0, 1, &State->QuadMesh->VertexBuffer, &Offset);
        vkCmdBindIndexBuffer(Commands.Buffer, State->QuadMesh->IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(Commands.Buffer, State->QuadMesh->NumIndices, 1, 0, 0, 0);
    }
    RenderTargetPassEnd(Commands);
}
