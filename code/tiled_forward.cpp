
/*

  TODO: Light List Idea

  - Can we reuse the light list from prev frame? Just recheck all the lights in the tiles they are currently in and update that way?
  I guess its not beneficial with just that because you still have to check all tiles in the map incase anything new got in there
  
*/

inline void TiledForwardSwapChainChange(tiled_forward_state* State, renderer_create_info CreateInfo, VkDescriptorSet* OutputRtSet)
{
    // IMPORTANT: Render Pass and PSO compilation has to happen here unless we want to precreate all render passes for each sample count
    //with all the PSOs at startup. Honestly, super annoying, DX12 probably doesn't have this annoying problem with render passes
    // TODO: DELETE OLD PSOS
    b32 ReCreate = State->RenderTargetArena.Used != 0;
    VkArenaClear(&State->RenderTargetArena);
    State->IsMsaaEnabled = CreateInfo.SampleCount != VK_SAMPLE_COUNT_1_BIT;
    
    // NOTE: Render Target Data
    {
        if (ReCreate)
        {
            RenderTargetDestroy(&State->DepthPrePass);
            RenderTargetDestroy(&State->ColorPass);
        }
        
        if (State->IsMsaaEnabled)
        {
            tiled_forward_msaa_targets* Targets = &State->MsaaTargets;

            // NOTE: MSAA Targets
            RenderTargetEntryReCreate(&State->RenderTargetArena, CreateInfo.Width, CreateInfo.Height, CreateInfo.ColorFormat,
                                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                                      CreateInfo.SampleCount, &Targets->MsaaColorImage, &Targets->MsaaColorEntry);
            RenderTargetEntryReCreate(&State->RenderTargetArena, CreateInfo.Width, CreateInfo.Height, VK_FORMAT_D32_SFLOAT,
                                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                      VK_IMAGE_ASPECT_DEPTH_BIT, CreateInfo.SampleCount, &Targets->MsaaDepthImage, &Targets->MsaaDepthEntry);

            // NOTE: Resolved Targets
            RenderTargetEntryReCreate(&State->RenderTargetArena, CreateInfo.Width, CreateInfo.Height, CreateInfo.ColorFormat,
                                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                                      &Targets->ResolvedColorImage, &Targets->ResolvedColorEntry);
            RenderTargetEntryReCreate(&State->RenderTargetArena, CreateInfo.Width, CreateInfo.Height, VK_FORMAT_R32_SFLOAT,
                                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                                      &Targets->ResolvedDepthImage, &Targets->ResolvedDepthEntry);
                        
            VkDescriptorImageWrite(&RenderState->DescriptorManager, State->ResolveDepthDescriptor, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                   Targets->MsaaDepthEntry.View, DemoState->PointSampler, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
            
            VkDescriptorImageWrite(&RenderState->DescriptorManager, *OutputRtSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                   Targets->ResolvedColorEntry.View, DemoState->LinearSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            VkDescriptorImageWrite(&RenderState->DescriptorManager, State->TiledForwardDescriptor, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                   Targets->ResolvedDepthEntry.View, DemoState->PointSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        else
        {
            tiled_forward_non_msaa_targets* Targets = &State->NonMsaaTargets;

            RenderTargetEntryReCreate(&State->RenderTargetArena, CreateInfo.Width, CreateInfo.Height, CreateInfo.ColorFormat,
                                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                                      &Targets->ColorImage, &Targets->ColorEntry);
            RenderTargetEntryReCreate(&State->RenderTargetArena, CreateInfo.Width, CreateInfo.Height, VK_FORMAT_D32_SFLOAT,
                                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                      VK_IMAGE_ASPECT_DEPTH_BIT, &Targets->DepthImage, &Targets->DepthEntry);

            VkDescriptorImageWrite(&RenderState->DescriptorManager, *OutputRtSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                   Targets->ColorEntry.View, DemoState->LinearSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            VkDescriptorImageWrite(&RenderState->DescriptorManager, State->TiledForwardDescriptor, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                   Targets->DepthEntry.View, DemoState->PointSampler, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
        }
    }

    // NOTE: Depth Pre Pass Data
    {
        // IMPORTANT: We don't do this in a single render pass since we cannot do compute between graphics
        if (State->IsMsaaEnabled)
        {
            // NOTE: MSAA Version
            tiled_forward_msaa_targets* Targets = &State->MsaaTargets;
            render_target_builder Builder = RenderTargetBuilderBegin(&DemoState->Arena, &DemoState->TempArena, CreateInfo.Width, CreateInfo.Height);
            RenderTargetAddTarget(&Builder, &Targets->MsaaDepthEntry, VkClearDepthStencilCreate(0, 0));
            RenderTargetAddTarget(&Builder, &Targets->ResolvedDepthEntry, VkClearDepthStencilCreate(0, 0));

            vk_render_pass_builder RpBuilder = VkRenderPassBuilderBegin(&DemoState->TempArena);
            u32 MsaaDepthId = VkRenderPassAttachmentAdd(&RpBuilder, Targets->MsaaDepthEntry.Format, CreateInfo.SampleCount,
                                                        VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
                                                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
            u32 DepthId = VkRenderPassAttachmentAdd(&RpBuilder, Targets->ResolvedDepthEntry.Format,
                                                    VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE,
                                                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            // NOTE: Depth PrePass
            VkRenderPassSubPassBegin(&RpBuilder, VK_PIPELINE_BIND_POINT_GRAPHICS);
            VkRenderPassDepthRefAdd(&RpBuilder, MsaaDepthId, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            VkRenderPassSubPassEnd(&RpBuilder);

            VkRenderPassDependency(&RpBuilder, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                   VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                   VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);

            // NOTE: Depth Resolve
            VkRenderPassSubPassBegin(&RpBuilder, VK_PIPELINE_BIND_POINT_GRAPHICS);
            VkRenderPassInputRefAdd(&RpBuilder, MsaaDepthId, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
            VkRenderPassColorRefAdd(&RpBuilder, DepthId, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            VkRenderPassSubPassEnd(&RpBuilder);

            VkRenderPassDependency(&RpBuilder, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);

            State->DepthPrePass = RenderTargetBuilderEnd(&Builder, VkRenderPassBuilderEnd(&RpBuilder, RenderState->Device));
        }
        else
        {
            // NOTE: Non MSAA Version
            tiled_forward_non_msaa_targets* Targets = &State->NonMsaaTargets;
            render_target_builder Builder = RenderTargetBuilderBegin(&DemoState->Arena, &DemoState->TempArena, CreateInfo.Width, CreateInfo.Height);
            RenderTargetAddTarget(&Builder, &Targets->DepthEntry, VkClearDepthStencilCreate(0, 0));
            
            vk_render_pass_builder RpBuilder = VkRenderPassBuilderBegin(&DemoState->TempArena);
            u32 DepthId = VkRenderPassAttachmentAdd(&RpBuilder, Targets->DepthEntry.Format, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                    VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED,
                                                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

            // NOTE: Depth PrePass
            VkRenderPassSubPassBegin(&RpBuilder, VK_PIPELINE_BIND_POINT_GRAPHICS);
            VkRenderPassDepthRefAdd(&RpBuilder, DepthId, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            VkRenderPassSubPassEnd(&RpBuilder);

            VkRenderPassDependency(&RpBuilder, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                   VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                   VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);

            State->DepthPrePass = RenderTargetBuilderEnd(&Builder, VkRenderPassBuilderEnd(&RpBuilder, RenderState->Device));
        }

        // NOTE: Depth PSO
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

            VkPipelineInputAssemblyAdd(&Builder, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
            VkPipelineDepthStateAdd(&Builder, VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER);
            VkPipelineMsaaStateSet(&Builder, CreateInfo.SampleCount, VK_FALSE);

            VkDescriptorSetLayout DescriptorLayouts[] =
                {
                    State->TiledForwardDescLayout,
                    CreateInfo.SceneDescLayout,
                };
            
            State->DepthPrePassPipeline = VkPipelineBuilderEnd(&Builder, RenderState->Device, &RenderState->PipelineManager,
                                                               State->DepthPrePass.RenderPass, 0, DescriptorLayouts, ArrayCount(DescriptorLayouts));
        }
                
        // NOTE: Resolve Depth PSO
        if (State->IsMsaaEnabled)
        {
            State->ResolveDepthPipeline = FullScreenResolveDepthCreate(State->DepthPrePass.RenderPass, 1);
        }
    }
    
    // NOTE: Tiled Forward
    {
        if (State->IsMsaaEnabled)
        {                            
            // NOTE: MSAA Version
            tiled_forward_msaa_targets* Targets = &State->MsaaTargets;

            render_target_builder Builder = RenderTargetBuilderBegin(&DemoState->Arena, &DemoState->TempArena, CreateInfo.Width, CreateInfo.Height);
            RenderTargetAddTarget(&Builder, &Targets->MsaaColorEntry, VkClearColorCreate(0, 0, 0, 1));
            RenderTargetAddTarget(&Builder, &Targets->ResolvedColorEntry, VkClearColorCreate(0, 0, 0, 1));
            RenderTargetAddTarget(&Builder, &Targets->MsaaDepthEntry, VkClearDepthStencilCreate(0, 0));
            
            vk_render_pass_builder RpBuilder = VkRenderPassBuilderBegin(&DemoState->TempArena);
            u32 MsaaColorId = VkRenderPassAttachmentAdd(&RpBuilder, Targets->MsaaColorEntry.Format, CreateInfo.SampleCount,
                                                        VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
                                                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            u32 ColorId = VkRenderPassAttachmentAdd(&RpBuilder, Targets->ResolvedColorEntry.Format, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                    VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED,
                                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            u32 MsaaDepthId = VkRenderPassAttachmentAdd(&RpBuilder, Targets->MsaaDepthEntry.Format, CreateInfo.SampleCount,
                                                        VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE,
                                                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                                                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

            VkRenderPassDependency(&RpBuilder, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                   VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);

            VkRenderPassSubPassBegin(&RpBuilder, VK_PIPELINE_BIND_POINT_GRAPHICS);
            VkRenderPassColorResolveRefAdd(&RpBuilder, MsaaColorId, ColorId, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            VkRenderPassDepthRefAdd(&RpBuilder, MsaaDepthId, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
            VkRenderPassSubPassEnd(&RpBuilder);

            State->ColorPass = RenderTargetBuilderEnd(&Builder, VkRenderPassBuilderEnd(&RpBuilder, RenderState->Device));
        }
        else
        {
            // NOTE: Non MSAA Version
            tiled_forward_non_msaa_targets* Targets = &State->NonMsaaTargets;

            render_target_builder Builder = RenderTargetBuilderBegin(&DemoState->Arena, &DemoState->TempArena, CreateInfo.Width, CreateInfo.Height);
            RenderTargetAddTarget(&Builder, &Targets->ColorEntry, VkClearColorCreate(0, 0, 0, 1));
            RenderTargetAddTarget(&Builder, &Targets->DepthEntry, VkClearDepthStencilCreate(0, 0));

            vk_render_pass_builder RpBuilder = VkRenderPassBuilderBegin(&DemoState->TempArena);

            u32 ColorId = VkRenderPassAttachmentAdd(&RpBuilder, Targets->ColorEntry.Format, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                    VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED,
                                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            u32 DepthId = VkRenderPassAttachmentAdd(&RpBuilder, Targets->DepthEntry.Format, VK_ATTACHMENT_LOAD_OP_LOAD,
                                                    VK_ATTACHMENT_STORE_OP_STORE,
                                                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                                                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

            VkRenderPassDependency(&RpBuilder, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                   VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);

            VkRenderPassSubPassBegin(&RpBuilder, VK_PIPELINE_BIND_POINT_GRAPHICS);
            VkRenderPassColorRefAdd(&RpBuilder, ColorId, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            VkRenderPassDepthRefAdd(&RpBuilder, DepthId, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
            VkRenderPassSubPassEnd(&RpBuilder);

            State->ColorPass = RenderTargetBuilderEnd(&Builder, VkRenderPassBuilderEnd(&RpBuilder, RenderState->Device));
        }

        // NOTE: Create Forward PSO
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

            VkPipelineInputAssemblyAdd(&Builder, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
            VkPipelineDepthStateAdd(&Builder, VK_TRUE, VK_FALSE, VK_COMPARE_OP_EQUAL);
            VkPipelineMsaaStateSet(&Builder, CreateInfo.SampleCount, VK_FALSE);
                
            // NOTE: Set the blending state
            VkPipelineColorAttachmentAdd(&Builder, VK_FALSE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO,
                                         VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);

            VkDescriptorSetLayout DescriptorLayouts[] =
                {
                    State->TiledForwardDescLayout,
                    CreateInfo.SceneDescLayout,
                    CreateInfo.MaterialDescLayout,
                };
            
            State->TiledForwardPipeline = VkPipelineBuilderEnd(&Builder, RenderState->Device, &RenderState->PipelineManager,
                                                               State->ColorPass.RenderPass, 0, DescriptorLayouts, ArrayCount(DescriptorLayouts));
        }
    }
    
    // NOTE: Tiled Data
    {
        u32 NumTilesX = CeilU32(f32(CreateInfo.Width) / f32(TILE_SIZE_IN_PIXELS));
        u32 NumTilesY = CeilU32(f32(CreateInfo.Height) / f32(TILE_SIZE_IN_PIXELS));

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

        VkDescriptorBufferWrite(&RenderState->DescriptorManager, State->TiledForwardDescriptor, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, State->GridFrustums);
        VkDescriptorImageWrite(&RenderState->DescriptorManager, State->TiledForwardDescriptor, 3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                               State->LightGrid_O.View, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL);
        VkDescriptorBufferWrite(&RenderState->DescriptorManager, State->TiledForwardDescriptor, 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, State->LightIndexList_O);
        VkDescriptorImageWrite(&RenderState->DescriptorManager, State->TiledForwardDescriptor, 6, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                               State->LightGrid_T.View, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL);
        VkDescriptorBufferWrite(&RenderState->DescriptorManager, State->TiledForwardDescriptor, 7, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, State->LightIndexList_T);
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

        // NOTE: Update our tiled forward globals
        {
            tiled_forward_globals* Data = VkTransferPushWriteStruct(&RenderState->TransferManager, State->TiledForwardGlobals, tiled_forward_globals,
                                                                    BarrierMask(VkAccessFlagBits(0), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT),
                                                                    BarrierMask(VK_ACCESS_UNIFORM_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
            *Data = {};
            Data->InverseProjection = Inverse(CameraGetP(&CreateInfo.Scene->Camera));
            Data->ScreenSize = V2(CreateInfo.Width, CreateInfo.Height);
            Data->GridSizeX = CeilU32(f32(CreateInfo.Width) / f32(TILE_SIZE_IN_PIXELS));
            Data->GridSizeY = CeilU32(f32(CreateInfo.Height) / f32(TILE_SIZE_IN_PIXELS));
        }
        VkTransferManagerFlush(&RenderState->TransferManager, RenderState->Device, RenderState->Commands.Buffer, &RenderState->BarrierManager);

        vkCmdBindPipeline(Commands.Buffer, VK_PIPELINE_BIND_POINT_COMPUTE, State->GridFrustumPipeline->Handle);
        VkDescriptorSet DescriptorSets[] =
            {
                State->TiledForwardDescriptor,
            };
        vkCmdBindDescriptorSets(Commands.Buffer, VK_PIPELINE_BIND_POINT_COMPUTE, State->GridFrustumPipeline->Layout, 0,
                                ArrayCount(DescriptorSets), DescriptorSets, 0, 0);
        u32 DispatchX = CeilU32(f32(CreateInfo.Width) / f32(8 * TILE_SIZE_IN_PIXELS));
        u32 DispatchY = CeilU32(f32(CreateInfo.Height) / f32(8 * TILE_SIZE_IN_PIXELS));
        vkCmdDispatch(Commands.Buffer, DispatchX, DispatchY, 1);
    }
    VkCommandsSubmit(RenderState->GraphicsQueue, Commands);
}

inline void TiledForwardCreate(renderer_create_info CreateInfo, VkDescriptorSet* OutputRtSet, tiled_forward_state* Result)
{
    u64 HeapSize = GigaBytes(1);
    Result->RenderTargetArena = VkLinearArenaCreate(VkMemoryAllocate(RenderState->Device, RenderState->LocalMemoryId, HeapSize), HeapSize);
    
    // NOTE: Create globals
    {
        Result->TiledForwardGlobals = VkBufferCreate(RenderState->Device, &RenderState->GpuArena, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                     sizeof(tiled_forward_globals));
        Result->LightIndexCounter_O = VkBufferCreate(RenderState->Device, &RenderState->GpuArena, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                     sizeof(u32));
        Result->LightIndexCounter_T = VkBufferCreate(RenderState->Device, &RenderState->GpuArena, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                     sizeof(u32));
        
        {
            vk_descriptor_layout_builder Builder = VkDescriptorLayoutBegin(&Result->TiledForwardDescLayout);
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

        Result->TiledForwardDescriptor = VkDescriptorSetAllocate(RenderState->Device, RenderState->DescriptorPool, Result->TiledForwardDescLayout);
        VkDescriptorBufferWrite(&RenderState->DescriptorManager, Result->TiledForwardDescriptor, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Result->TiledForwardGlobals);
        VkDescriptorBufferWrite(&RenderState->DescriptorManager, Result->TiledForwardDescriptor, 5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Result->LightIndexCounter_O);
        VkDescriptorBufferWrite(&RenderState->DescriptorManager, Result->TiledForwardDescriptor, 8, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Result->LightIndexCounter_T);
    }

    // NOTE: Grid Frustum
    {
        VkDescriptorSetLayout Layouts[] =
            {
                Result->TiledForwardDescLayout,
            };
            
        Result->GridFrustumPipeline = VkPipelineComputeCreate(RenderState->Device, &RenderState->PipelineManager, &DemoState->TempArena,
                                                              "shader_tiled_forward_grid_frustum.spv", "main", Layouts, ArrayCount(Layouts));
    }

    // NOTE: Resolve Depth Descriptor
    Result->ResolveDepthDescriptor = VkDescriptorSetAllocate(RenderState->Device, RenderState->DescriptorPool, RenderState->ResolveDepthDescLayout);
        
    TiledForwardSwapChainChange(Result, CreateInfo, OutputRtSet);
        
    // NOTE: Light Cull
    {
        VkDescriptorSetLayout Layouts[] =
            {
                Result->TiledForwardDescLayout,
                CreateInfo.SceneDescLayout,
            };
            
        Result->LightCullPipeline = VkPipelineComputeCreate(RenderState->Device, &RenderState->PipelineManager, &DemoState->TempArena,
                                                            "shader_tiled_forward_light_culling.spv", "main", Layouts, ArrayCount(Layouts));
    }
}

inline void TiledForwardRender(vk_commands Commands, tiled_forward_state* State, render_scene* Scene)
{
    // NOTE: Clear buffers and upload data
    {
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

    RenderTargetPassBegin(&State->DepthPrePass, Commands, RenderTargetRenderPass_SetViewPort | RenderTargetRenderPass_SetScissor);
    // NOTE: Depth Pre Pass
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

        for (u32 InstanceId = 0; InstanceId < Scene->NumOpaqueInstances; ++InstanceId)
        {
            instance_entry* CurrInstance = Scene->OpaqueInstances + InstanceId;
            render_mesh* CurrMesh = Scene->RenderMeshes + CurrInstance->MeshId;
            
            VkDeviceSize Offset = 0;
            vkCmdBindVertexBuffers(Commands.Buffer, 0, 1, &CurrMesh->VertexBuffer, &Offset);
            vkCmdBindIndexBuffer(Commands.Buffer, CurrMesh->IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(Commands.Buffer, CurrMesh->NumIndices, 1, 0, 0, InstanceId);
        }
    }

    if (State->IsMsaaEnabled)
    {
        RenderTargetNextSubPass(Commands);
        // NOTE: Depth Resolve
        FullScreenPassRender(Commands, State->ResolveDepthPipeline, 1, &State->ResolveDepthDescriptor);
    }
    
    RenderTargetPassEnd(Commands);
    
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
    RenderTargetPassBegin(&State->ColorPass, Commands, RenderTargetRenderPass_SetViewPort | RenderTargetRenderPass_SetScissor);
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
        
        for (u32 InstanceId = 0; InstanceId < Scene->NumOpaqueInstances; ++InstanceId)
        {
            instance_entry* CurrInstance = Scene->OpaqueInstances + InstanceId;
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
    RenderTargetPassEnd(Commands);
}
