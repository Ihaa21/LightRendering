
/*

   TODO: Light List Idea

     - Can we reuse the light list from prev frame? Just recheck all the lights in the tiles they are currently in and update that way?
       I guess its not beneficial with just that because you still have to check all tiles in the map incase anything new got in there
  
 */

inline tiled_forward_state TiledForwardCreate(renderer_create_info CreateInfo, vk_commands Commands, VkDescriptorSet* OutputRtSet)
{
    tiled_forward_state Result = {};

    Result.ColorEntry = RenderTargetEntryCreate(&RenderState->GpuArena, CreateInfo.Width, CreateInfo.Height,
                                                CreateInfo.ColorFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                VK_IMAGE_ASPECT_COLOR_BIT);
    Result.DepthEntry = RenderTargetEntryCreate(&RenderState->GpuArena, CreateInfo.Width, CreateInfo.Height,
                                                VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                VK_IMAGE_ASPECT_DEPTH_BIT);

    VkDescriptorImageWrite(&RenderState->DescriptorManager, *OutputRtSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                           Result.ColorEntry.View, DemoState->LinearSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    // NOTE: Create globals
    {
        u32 NumTilesX = CeilU32(f32(CreateInfo.Width) / f32(TILE_SIZE_IN_PIXELS));
        u32 NumTilesY = CeilU32(f32(CreateInfo.Height) / f32(TILE_SIZE_IN_PIXELS));
        
        Result.TiledForwardGlobals = VkBufferCreate(RenderState->Device, &RenderState->GpuArena, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                    sizeof(tiled_forward_globals));
        Result.GridFrustums = VkBufferCreate(RenderState->Device, &RenderState->GpuArena, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                             sizeof(frustum) * NumTilesX * NumTilesY);
        Result.LightIndexList_O = VkBufferCreate(RenderState->Device, &RenderState->GpuArena, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                 sizeof(u32) * MAX_LIGHTS_PER_TILE * NumTilesX * NumTilesY);
        Result.LightIndexCounter_O = VkBufferCreate(RenderState->Device, &RenderState->GpuArena, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                    sizeof(u32));
        Result.LightGrid_O = VkImage2dCreate(RenderState->Device, &RenderState->GpuArena, NumTilesX, NumTilesY, VK_FORMAT_R32G32_UINT,
                                             VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        Result.LightIndexList_T = VkBufferCreate(RenderState->Device, &RenderState->GpuArena, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                 sizeof(u32) * MAX_LIGHTS_PER_TILE * NumTilesX * NumTilesY);
        Result.LightIndexCounter_T = VkBufferCreate(RenderState->Device, &RenderState->GpuArena, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                    sizeof(u32));
        Result.LightGrid_T = VkImage2dCreate(RenderState->Device, &RenderState->GpuArena, NumTilesX, NumTilesY, VK_FORMAT_R32G32_UINT,
                                             VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        
        {
            vk_descriptor_layout_builder Builder = VkDescriptorLayoutBegin(&Result.TiledForwardDescLayout);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            VkDescriptorLayoutEnd(RenderState->Device, &Builder);
        }

        Result.TiledForwardDescriptor = VkDescriptorSetAllocate(RenderState->Device, RenderState->DescriptorPool, Result.TiledForwardDescLayout);
        VkDescriptorBufferWrite(&RenderState->DescriptorManager, Result.TiledForwardDescriptor, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Result.TiledForwardGlobals);
        VkDescriptorBufferWrite(&RenderState->DescriptorManager, Result.TiledForwardDescriptor, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Result.GridFrustums);
        VkDescriptorImageWrite(&RenderState->DescriptorManager, Result.TiledForwardDescriptor, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                               Result.DepthEntry.View, DemoState->PointSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        VkDescriptorImageWrite(&RenderState->DescriptorManager, Result.TiledForwardDescriptor, 3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                               Result.LightGrid_O.View, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL);
        VkDescriptorBufferWrite(&RenderState->DescriptorManager, Result.TiledForwardDescriptor, 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Result.LightIndexList_O);
        VkDescriptorBufferWrite(&RenderState->DescriptorManager, Result.TiledForwardDescriptor, 5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Result.LightIndexCounter_O);
        VkDescriptorImageWrite(&RenderState->DescriptorManager, Result.TiledForwardDescriptor, 6, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                               Result.LightGrid_T.View, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL);
        VkDescriptorBufferWrite(&RenderState->DescriptorManager, Result.TiledForwardDescriptor, 7, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Result.LightIndexList_T);
        VkDescriptorBufferWrite(&RenderState->DescriptorManager, Result.TiledForwardDescriptor, 8, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Result.LightIndexCounter_T);
    }
    
    // NOTE: Create PSOs
    // IMPORTANT: We don't do this in a single render pass since we cannot do compute between graphics
    {
        // NOTE: Grid Frustum
        {
            VkDescriptorSetLayout Layouts[] =
            {
                Result.TiledForwardDescLayout,
            };
            
            Result.GridFrustumPipeline = VkPipelineComputeCreate(RenderState->Device, &RenderState->PipelineManager, &DemoState->TempArena,
                                                                 "shader_grid_frustum.spv", "main", Layouts, ArrayCount(Layouts));
        }

        // NOTE: Depth Pre Pass
        {
            // NOTE: RT
            {
                render_target_builder Builder = RenderTargetBuilderBegin(&DemoState->Arena, &DemoState->TempArena, CreateInfo.Width, CreateInfo.Height);
                RenderTargetAddTarget(&Builder, &Result.DepthEntry, VkClearDepthStencilCreate(1, 0));
                            
                vk_render_pass_builder RpBuilder = VkRenderPassBuilderBegin(&DemoState->TempArena);

                u32 DepthId = VkRenderPassAttachmentAdd(&RpBuilder, Result.DepthEntry.Format, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                        VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED,
                                                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

                VkRenderPassSubPassBegin(&RpBuilder, VK_PIPELINE_BIND_POINT_GRAPHICS);
                VkRenderPassDepthRefAdd(&RpBuilder, DepthId, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
                VkRenderPassSubPassEnd(&RpBuilder);

                Result.DepthPrePass = RenderTargetBuilderEnd(&Builder, VkRenderPassBuilderEnd(&RpBuilder, RenderState->Device));
            }

            {
                vk_pipeline_builder Builder = VkPipelineBuilderBegin(&DemoState->TempArena);

                // NOTE: Shaders
                VkPipelineShaderAdd(&Builder, "shader_tiled_forward_vert.spv", "main", VK_SHADER_STAGE_VERTEX_BIT);
                
                // NOTE: Specify input vertex data format
                VkPipelineVertexBindingBegin(&Builder);
                VkPipelineVertexAttributeAdd(&Builder, VK_FORMAT_R32G32B32_SFLOAT, sizeof(v3));
                VkPipelineVertexAttributeAdd(&Builder, VK_FORMAT_R32G32B32_SFLOAT, sizeof(v3));
                VkPipelineVertexAttributeAdd(&Builder, VK_FORMAT_R32G32_SFLOAT, sizeof(v2));
                VkPipelineVertexBindingEnd(&Builder);

                VkPipelineInputAssemblyAdd(&Builder, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, VK_FALSE);
                VkPipelineDepthStateAdd(&Builder, VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);

                VkDescriptorSetLayout DescriptorLayouts[] =
                    {
                        Result.TiledForwardDescLayout,
                        CreateInfo.SceneDescLayout,
                    };
            
                Result.DepthPrePassPipeline = VkPipelineBuilderEnd(&Builder, RenderState->Device, &RenderState->PipelineManager,
                                                                   Result.DepthPrePass.RenderPass, 0, DescriptorLayouts, ArrayCount(DescriptorLayouts));

            }
        }
        
        // NOTE: Light Cull
        {
            VkDescriptorSetLayout Layouts[] =
            {
                Result.TiledForwardDescLayout,
                CreateInfo.SceneDescLayout,
            };
            
            Result.LightCullPipeline = VkPipelineComputeCreate(RenderState->Device, &RenderState->PipelineManager, &DemoState->TempArena,
                                                               "shader_light_culling.spv", "main", Layouts, ArrayCount(Layouts));
        }

        // NOTE: Tiled Forward 
        {
            // NOTE: RT
            {
                render_target_builder Builder = RenderTargetBuilderBegin(&DemoState->Arena, &DemoState->TempArena, CreateInfo.Width, CreateInfo.Height);
                RenderTargetAddTarget(&Builder, &Result.ColorEntry, VkClearColorCreate(0, 0, 0, 1));
                RenderTargetAddTarget(&Builder, &Result.DepthEntry, VkClearDepthStencilCreate(1, 0));
                            
                vk_render_pass_builder RpBuilder = VkRenderPassBuilderBegin(&DemoState->TempArena);

                u32 ColorId = VkRenderPassAttachmentAdd(&RpBuilder, Result.ColorEntry.Format, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                        VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED,
                                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                u32 DepthId = VkRenderPassAttachmentAdd(&RpBuilder, Result.DepthEntry.Format, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                        VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED,
                                                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

                VkRenderPassSubPassBegin(&RpBuilder, VK_PIPELINE_BIND_POINT_GRAPHICS);
                VkRenderPassColorRefAdd(&RpBuilder, ColorId, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                VkRenderPassDepthRefAdd(&RpBuilder, DepthId, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
                VkRenderPassSubPassEnd(&RpBuilder);

                Result.ColorPass = RenderTargetBuilderEnd(&Builder, VkRenderPassBuilderEnd(&RpBuilder, RenderState->Device));
            }

            {
                vk_pipeline_builder Builder = VkPipelineBuilderBegin(&DemoState->TempArena);

                // NOTE: Shaders
                VkPipelineShaderAdd(&Builder, "shader_tiled_forward_vert.spv", "main", VK_SHADER_STAGE_VERTEX_BIT);
                VkPipelineShaderAdd(&Builder, "shader_tiled_forward_frag.spv", "main", VK_SHADER_STAGE_FRAGMENT_BIT);
                
                // NOTE: Specify input vertex data format
                VkPipelineVertexBindingBegin(&Builder);
                VkPipelineVertexAttributeAdd(&Builder, VK_FORMAT_R32G32B32_SFLOAT, sizeof(v3));
                VkPipelineVertexAttributeAdd(&Builder, VK_FORMAT_R32G32B32_SFLOAT, sizeof(v3));
                VkPipelineVertexAttributeAdd(&Builder, VK_FORMAT_R32G32_SFLOAT, sizeof(v2));
                VkPipelineVertexBindingEnd(&Builder);

                VkPipelineInputAssemblyAdd(&Builder, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, VK_FALSE);
                VkPipelineDepthStateAdd(&Builder, VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
                
                // NOTE: Set the blending state
                VkPipelineColorAttachmentAdd(&Builder, VK_FALSE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO,
                                             VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);

                VkDescriptorSetLayout DescriptorLayouts[] =
                    {
                        Result.TiledForwardDescLayout,
                        CreateInfo.SceneDescLayout,
                        CreateInfo.MaterialDescLayout,
                    };
            
                Result.TiledForwardPipeline = VkPipelineBuilderEnd(&Builder, RenderState->Device, &RenderState->PipelineManager,
                                                                   Result.ColorPass.RenderPass, 0, DescriptorLayouts, ArrayCount(DescriptorLayouts));
            }
        }
    }

    // NOTE: Init Grid Frustums
    {
        // NOTE: Init our images
        VkBarrierImageAdd(&RenderState->BarrierManager, VK_ACCESS_MEMORY_READ_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_MEMORY_READ_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_IMAGE_LAYOUT_GENERAL,
                          VK_IMAGE_ASPECT_COLOR_BIT, Result.LightGrid_O.Image);
        VkBarrierImageAdd(&RenderState->BarrierManager, VK_ACCESS_MEMORY_READ_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_MEMORY_READ_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_IMAGE_LAYOUT_GENERAL,
                          VK_IMAGE_ASPECT_COLOR_BIT, Result.LightGrid_T.Image);
        VkBarrierManagerFlush(&RenderState->BarrierManager, Commands.Buffer);

        // TODO: Figure out the spaces of lights and re enable this
#if 0
        // NOTE: Update our tiled forward globals
        {
            tiled_forward_globals* Data = VkTransferPushWriteStruct(&RenderState->TransferManager, Result.TiledForwardGlobals, tiled_forward_globals,
                                                                    BarrierMask(VkAccessFlagBits(0), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT),
                                                                    BarrierMask(VK_ACCESS_UNIFORM_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
            *Data = {};
            Data->InverseProjection = Inverse(CameraGetVP(&Scene->Camera));
            Data->ScreenSize = V2(RenderState->WindowWidth, RenderState->WindowHeight);
            Data->GridSizeX = CeilU32(f32(RenderState->WindowWidth) / f32(TILE_SIZE_IN_PIXELS));
            Data->GridSizeY = CeilU32(f32(RenderState->WindowHeight) / f32(TILE_SIZE_IN_PIXELS));
        }
        VkDescriptorManagerFlush(RenderState->Device, &RenderState->DescriptorManager);
        VkTransferManagerFlush(&RenderState->TransferManager, RenderState->Device, RenderState->Commands.Buffer, &RenderState->BarrierManager);

        vkCmdBindPipeline(Commands.Buffer, VK_PIPELINE_BIND_POINT_COMPUTE, Result.GridFrustumPipeline->Handle);
        VkDescriptorSet DescriptorSets[] =
            {
                Result.TiledForwardDescriptor,
            };
        vkCmdBindDescriptorSets(Commands.Buffer, VK_PIPELINE_BIND_POINT_COMPUTE, Result.GridFrustumPipeline->Layout, 0,
                                ArrayCount(DescriptorSets), DescriptorSets, 0, 0);
        u32 DispatchX = CeilU32(f32(RenderState->WindowWidth) / f32(TILE_SIZE_IN_PIXELS * TILE_SIZE_IN_PIXELS));
        u32 DispatchY = CeilU32(f32(RenderState->WindowHeight) / f32(TILE_SIZE_IN_PIXELS * TILE_SIZE_IN_PIXELS));
        vkCmdDispatch(Commands.Buffer, DispatchX, DispatchY, 1);
#endif
    }
    
    return Result;
}

inline void TiledForwardPreRender(vk_commands Commands, tiled_forward_state* State, render_scene* Scene)
{
    // TODO: Transfer Manager needs to be given support for multiple uploads in the same cmd buffer + expandable and freeing
    // the memory. For now we put it here to avoid it
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
        
    // TODO: Probably add better support for transfer updates? Idk this might be fine
    {
        tiled_forward_globals* Data = VkTransferPushWriteStruct(&RenderState->TransferManager, State->TiledForwardGlobals, tiled_forward_globals,
                                                                BarrierMask(VkAccessFlagBits(0), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT),
                                                                BarrierMask(VK_ACCESS_UNIFORM_READ_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT));
        *Data = {};
        Data->InverseProjection = Inverse(CameraGetP(&Scene->Camera));
        Data->ScreenSize = V2(RenderState->WindowWidth, RenderState->WindowHeight);
        Data->GridSizeX = CeilU32(f32(RenderState->WindowWidth) / f32(TILE_SIZE_IN_PIXELS));
        Data->GridSizeY = CeilU32(f32(RenderState->WindowHeight) / f32(TILE_SIZE_IN_PIXELS));

        // TODO: Appears that inverse doesn't gen the same as online results
        m4 Test = CameraGetVP(&Scene->Camera) * Data->InverseProjection;
        int i = 0;
    }
}

inline void TiledForwardRender(vk_commands Commands, tiled_forward_state* State, render_scene* Scene)
{
    // NOTE: Grid Frustum // TODO: This should only happen at init
    {        
        vkCmdBindPipeline(Commands.Buffer, VK_PIPELINE_BIND_POINT_COMPUTE, State->GridFrustumPipeline->Handle);
        VkDescriptorSet DescriptorSets[] =
            {
                State->TiledForwardDescriptor,
            };
        vkCmdBindDescriptorSets(Commands.Buffer, VK_PIPELINE_BIND_POINT_COMPUTE, State->GridFrustumPipeline->Layout, 0,
                                ArrayCount(DescriptorSets), DescriptorSets, 0, 0);
        u32 DispatchX = CeilU32(f32(RenderState->WindowWidth) / f32(8 * TILE_SIZE_IN_PIXELS));
        u32 DispatchY = CeilU32(f32(RenderState->WindowHeight) / f32(8 * TILE_SIZE_IN_PIXELS));
        vkCmdDispatch(Commands.Buffer, DispatchX, DispatchY, 1);
    }
        
    
    // NOTE: Depth Pre Pass
    RenderTargetRenderPassBegin(&State->DepthPrePass, Commands, RenderTargetRenderPass_SetViewPort | RenderTargetRenderPass_SetScissor);
    {
        vkCmdBindPipeline(Commands.Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, State->DepthPrePassPipeline->Handle);
        {
            VkDescriptorSet DescriptorSets[] =
                {
                    Scene->SceneDescriptor,
                };
            vkCmdBindDescriptorSets(Commands.Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, State->DepthPrePassPipeline->Layout, 1,
                                    ArrayCount(DescriptorSets), DescriptorSets, 0, 0);
        }

        for (u32 InstanceId = 0; InstanceId < Scene->NumInstances; ++InstanceId)
        {
            instance_entry* CurrInstance = Scene->Instances + InstanceId;
            render_mesh* CurrMesh = Scene->RenderMeshes + CurrInstance->MeshId;
            
            VkDeviceSize Offset = 0;
            vkCmdBindVertexBuffers(Commands.Buffer, 0, 1, &CurrMesh->VertexBuffer, &Offset);
            vkCmdBindIndexBuffer(Commands.Buffer, CurrMesh->IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(Commands.Buffer, CurrMesh->NumIndices, 1, 0, 0, InstanceId);
        }
    }
    RenderTargetRenderPassEnd(Commands);

    // NOTE: Transition depth buffer for reading
    VkBarrierImageAdd(&RenderState->BarrierManager, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                      VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                      VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_IMAGE_ASPECT_DEPTH_BIT, State->DepthEntry.Image);
    VkBarrierManagerFlush(&RenderState->BarrierManager, Commands.Buffer);
    
    // NOTE: Light Culling Pass
    {
        vkCmdBindPipeline(Commands.Buffer, VK_PIPELINE_BIND_POINT_COMPUTE, State->LightCullPipeline->Handle);
        VkDescriptorSet DescriptorSets[] =
            {
                State->TiledForwardDescriptor,
                Scene->SceneDescriptor,
            };
        vkCmdBindDescriptorSets(Commands.Buffer, VK_PIPELINE_BIND_POINT_COMPUTE, State->LightCullPipeline->Layout, 0,
                                ArrayCount(DescriptorSets), DescriptorSets, 0, 0);
        u32 DispatchX = CeilU32(f32(RenderState->WindowWidth) / f32(TILE_SIZE_IN_PIXELS));
        u32 DispatchY = CeilU32(f32(RenderState->WindowHeight) / f32(TILE_SIZE_IN_PIXELS));
        vkCmdDispatch(Commands.Buffer, DispatchX, DispatchY, 1);
    }

    // NOTE: Color Pass
    RenderTargetRenderPassBegin(&State->ColorPass, Commands, RenderTargetRenderPass_SetViewPort | RenderTargetRenderPass_SetScissor);
    {
        vkCmdBindPipeline(Commands.Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, State->TiledForwardPipeline->Handle);
        {
            VkDescriptorSet DescriptorSets[] =
                {
                    State->TiledForwardDescriptor,
                    Scene->SceneDescriptor,
                };
            vkCmdBindDescriptorSets(Commands.Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, State->TiledForwardPipeline->Layout, 0,
                                    ArrayCount(DescriptorSets), DescriptorSets, 0, 0);
        }
        
        for (u32 InstanceId = 0; InstanceId < Scene->NumInstances; ++InstanceId)
        {
            instance_entry* CurrInstance = Scene->Instances + InstanceId;
            render_mesh* CurrMesh = Scene->RenderMeshes + CurrInstance->MeshId;

            {
                VkDescriptorSet DescriptorSets[] =
                    {
                        CurrMesh->MaterialDescriptor,
                    };
                vkCmdBindDescriptorSets(Commands.Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, State->TiledForwardPipeline->Layout, 2,
                                        ArrayCount(DescriptorSets), DescriptorSets, 0, 0);
            }
            
            VkDeviceSize Offset = 0;
            vkCmdBindVertexBuffers(Commands.Buffer, 0, 1, &CurrMesh->VertexBuffer, &Offset);
            vkCmdBindIndexBuffer(Commands.Buffer, CurrMesh->IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(Commands.Buffer, CurrMesh->NumIndices, 1, 0, 0, InstanceId);
        }
    }
    RenderTargetRenderPassEnd(Commands);
}
