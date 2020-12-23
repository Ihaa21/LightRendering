
inline void DeferredSwapChainChange(deferred_state* State, u32 Width, u32 Height, VkFormat ColorFormat, render_scene* Scene,
                                    VkDescriptorSet* OutputRtSet)
{
    b32 ReCreate = State->RenderTargetArena.Used != 0;
    VkArenaClear(&State->RenderTargetArena);

    // NOTE: Render Target Data
    {
        // TODO: We need sampled bit as well as input attachment? Doesn't one imply the other?
        VkImageLayout GBufferLayout = VkImageLayout(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        RenderTargetEntryReCreate(&State->RenderTargetArena, Width, Height, VK_FORMAT_R32G32B32A32_SFLOAT,
                                  GBufferLayout, VK_IMAGE_ASPECT_COLOR_BIT, &State->GBufferPositionEntry);
        RenderTargetEntryReCreate(&State->RenderTargetArena, Width, Height, VK_FORMAT_R16G16B16A16_SNORM, GBufferLayout,
                                  VK_IMAGE_ASPECT_COLOR_BIT, &State->GBufferNormalEntry);
        RenderTargetEntryReCreate(&State->RenderTargetArena, Width, Height, VK_FORMAT_R8G8B8A8_UNORM, GBufferLayout,
                                  VK_IMAGE_ASPECT_COLOR_BIT, &State->GBufferColorEntry);
        RenderTargetEntryReCreate(&State->RenderTargetArena, Width, Height, ColorFormat,
                                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                                  &State->OutColorEntry);
        RenderTargetEntryReCreate(&State->RenderTargetArena, Width, Height, VK_FORMAT_D32_SFLOAT,
                                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, &State->DepthEntry);

        if (ReCreate)
        {
            RenderTargetUpdateEntries(&DemoState->TempArena, &State->RenderTarget);
        }
        
        VkDescriptorImageWrite(&RenderState->DescriptorManager, State->GBufferDescriptor, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                               State->GBufferPositionEntry.View, DemoState->PointSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        VkDescriptorImageWrite(&RenderState->DescriptorManager, State->GBufferDescriptor, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                               State->GBufferNormalEntry.View, DemoState->PointSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        VkDescriptorImageWrite(&RenderState->DescriptorManager, State->GBufferDescriptor, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                               State->GBufferColorEntry.View, DemoState->PointSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        VkDescriptorImageWrite(&RenderState->DescriptorManager, *OutputRtSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                               State->OutColorEntry.View, DemoState->LinearSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    VkDescriptorManagerFlush(RenderState->Device, &RenderState->DescriptorManager);
}

inline void DeferredCreate(renderer_create_info CreateInfo, VkDescriptorSet* OutputRtSet, deferred_state* Result)
{
    *Result = {};

    u64 HeapSize = GigaBytes(1);
    Result->RenderTargetArena = VkLinearArenaCreate(VkMemoryAllocate(RenderState->Device, RenderState->LocalMemoryId, HeapSize), HeapSize);

    // NOTE: GBuffer Descriptor Set
    {
        {
            vk_descriptor_layout_builder Builder = VkDescriptorLayoutBegin(&Result->GBufferDescLayout);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
            VkDescriptorLayoutAdd(&Builder, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
            VkDescriptorLayoutEnd(RenderState->Device, &Builder);
        }

        Result->GBufferDescriptor = VkDescriptorSetAllocate(RenderState->Device, RenderState->DescriptorPool, Result->GBufferDescLayout);
    }

    DeferredSwapChainChange(Result, CreateInfo.Width, CreateInfo.Height, CreateInfo.ColorFormat, CreateInfo.Scene, OutputRtSet);
    
    // NOTE: RT
    {
        render_target_builder Builder = RenderTargetBuilderBegin(&DemoState->Arena, &DemoState->TempArena, CreateInfo.Width, CreateInfo.Height);
        RenderTargetAddTarget(&Builder, &Result->GBufferPositionEntry, VkClearColorCreate(0, 0, 0, 0));
        RenderTargetAddTarget(&Builder, &Result->GBufferNormalEntry, VkClearColorCreate(0, 0, 0, 0));
        RenderTargetAddTarget(&Builder, &Result->GBufferColorEntry, VkClearColorCreate(0, 0, 0, 1));
        RenderTargetAddTarget(&Builder, &Result->OutColorEntry, VkClearColorCreate(0, 0, 0, 1));
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

        u32 OutColorId = VkRenderPassAttachmentAdd(&RpBuilder, Result->OutColorEntry.Format, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                   VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED,
                                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        u32 DepthId = VkRenderPassAttachmentAdd(&RpBuilder, Result->DepthEntry.Format, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED,
                                                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

        VkRenderPassSubPassBegin(&RpBuilder, VK_PIPELINE_BIND_POINT_GRAPHICS);
        VkRenderPassColorRefAdd(&RpBuilder, GBufferPositionId, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        VkRenderPassColorRefAdd(&RpBuilder, GBufferNormalId, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        VkRenderPassColorRefAdd(&RpBuilder, GBufferColorId, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        VkRenderPassDepthRefAdd(&RpBuilder, DepthId, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        VkRenderPassSubPassEnd(&RpBuilder);

        VkRenderPassDependency(&RpBuilder, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                               VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);
        
        VkRenderPassSubPassBegin(&RpBuilder, VK_PIPELINE_BIND_POINT_GRAPHICS);
        VkRenderPassInputRefAdd(&RpBuilder, GBufferPositionId, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        VkRenderPassInputRefAdd(&RpBuilder, GBufferNormalId, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        VkRenderPassInputRefAdd(&RpBuilder, GBufferColorId, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        VkRenderPassColorRefAdd(&RpBuilder, OutColorId, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        VkRenderPassDepthRefAdd(&RpBuilder, DepthId, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        VkRenderPassSubPassEnd(&RpBuilder);

        Result->RenderTarget = RenderTargetBuilderEnd(&Builder, VkRenderPassBuilderEnd(&RpBuilder, RenderState->Device));
    }
    
    // NOTE: Create PSOs
    {
        // NOTE: GBuffer PSO
        {
            vk_pipeline_builder Builder = VkPipelineBuilderBegin(&DemoState->TempArena);

            // NOTE: Shaders
            VkPipelineShaderAdd(&Builder, "shader_deferred_gbuffer_vert.spv", "main", VK_SHADER_STAGE_VERTEX_BIT);
            VkPipelineShaderAdd(&Builder, "shader_deferred_gbuffer_frag.spv", "main", VK_SHADER_STAGE_FRAGMENT_BIT);
                
            // NOTE: Specify input vertex data format
            VkPipelineVertexBindingBegin(&Builder);
            VkPipelineVertexAttributeAdd(&Builder, VK_FORMAT_R32G32B32_SFLOAT, sizeof(v3));
            VkPipelineVertexAttributeAdd(&Builder, VK_FORMAT_R32G32B32_SFLOAT, sizeof(v3));
            VkPipelineVertexAttributeAdd(&Builder, VK_FORMAT_R32G32_SFLOAT, sizeof(v2));
            VkPipelineVertexBindingEnd(&Builder);

            VkPipelineInputAssemblyAdd(&Builder, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
            VkPipelineDepthStateAdd(&Builder, VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER);
                
            // NOTE: Set the blending state
            VkPipelineColorAttachmentAdd(&Builder, VK_FALSE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO,
                                         VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);
            VkPipelineColorAttachmentAdd(&Builder, VK_FALSE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO,
                                         VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);
            VkPipelineColorAttachmentAdd(&Builder, VK_FALSE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO,
                                         VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);

            VkDescriptorSetLayout DescriptorLayouts[] =
                {
                    CreateInfo.MaterialDescLayout,
                    CreateInfo.SceneDescLayout,
                };
            
            Result->GBufferPipeline = VkPipelineBuilderEnd(&Builder, RenderState->Device, &RenderState->PipelineManager,
                                                           Result->RenderTarget.RenderPass, 0, DescriptorLayouts, ArrayCount(DescriptorLayouts));
        }

        // TODO: Handle correctly lights which contain the camera
        // NOTE: Point Light PSO
        {
            vk_pipeline_builder Builder = VkPipelineBuilderBegin(&DemoState->TempArena);

            // NOTE: Shaders
            VkPipelineShaderAdd(&Builder, "shader_deferred_point_light_vert.spv", "main", VK_SHADER_STAGE_VERTEX_BIT);
            VkPipelineShaderAdd(&Builder, "shader_deferred_point_light_frag.spv", "main", VK_SHADER_STAGE_FRAGMENT_BIT);
                
            // NOTE: Specify input vertex data format
            VkPipelineVertexBindingBegin(&Builder);
            VkPipelineVertexAttributeAdd(&Builder, VK_FORMAT_R32G32B32_SFLOAT, 2*sizeof(v3) + sizeof(v2));
            VkPipelineVertexBindingEnd(&Builder);

            VkPipelineInputAssemblyAdd(&Builder, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
            VkPipelineRasterizationStateSet(&Builder, VK_FALSE, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
            VkPipelineDepthStateAdd(&Builder, VK_TRUE, VK_FALSE, VK_COMPARE_OP_GREATER);
                
            // NOTE: Set the blending state
            VkPipelineColorAttachmentAdd(&Builder, VK_TRUE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE,
                                         VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);

            VkDescriptorSetLayout DescriptorLayouts[] =
                {
                    CreateInfo.MaterialDescLayout,
                    CreateInfo.SceneDescLayout,
                    Result->GBufferDescLayout,
                };
            
            Result->PointLightPipeline = VkPipelineBuilderEnd(&Builder, RenderState->Device, &RenderState->PipelineManager,
                                                              Result->RenderTarget.RenderPass, 1, DescriptorLayouts, ArrayCount(DescriptorLayouts));
        }

        // NOTE: Directional Light PSO
        {
            vk_pipeline_builder Builder = VkPipelineBuilderBegin(&DemoState->TempArena);

            // NOTE: Shaders
            VkPipelineShaderAdd(&Builder, "shader_deferred_directional_light_vert.spv", "main", VK_SHADER_STAGE_VERTEX_BIT);
            VkPipelineShaderAdd(&Builder, "shader_deferred_directional_light_frag.spv", "main", VK_SHADER_STAGE_FRAGMENT_BIT);
                
            // NOTE: Specify input vertex data format
            VkPipelineVertexBindingBegin(&Builder);
            VkPipelineVertexAttributeAdd(&Builder, VK_FORMAT_R32G32B32_SFLOAT, 2*sizeof(v3) + sizeof(v2));
            VkPipelineVertexBindingEnd(&Builder);

            VkPipelineInputAssemblyAdd(&Builder, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
                
            // NOTE: Set the blending state
            VkPipelineColorAttachmentAdd(&Builder, VK_TRUE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE,
                                         VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);

            VkDescriptorSetLayout DescriptorLayouts[] =
                {
                    CreateInfo.MaterialDescLayout,
                    CreateInfo.SceneDescLayout,
                    Result->GBufferDescLayout,
                };
            
            Result->DirectionalLightPipeline = VkPipelineBuilderEnd(&Builder, RenderState->Device, &RenderState->PipelineManager,
                                                                    Result->RenderTarget.RenderPass, 1, DescriptorLayouts, ArrayCount(DescriptorLayouts));
        }
    }
}

inline void DeferredAddMeshes(deferred_state* DeferredState, render_mesh* QuadMesh, render_mesh* SphereMesh)
{
    DeferredState->QuadMesh = QuadMesh;
    DeferredState->SphereMesh = SphereMesh;
}

inline void DeferredRender(vk_commands Commands, deferred_state* DeferredState, render_scene* Scene)
{
    RenderTargetRenderPassBegin(&DeferredState->RenderTarget, Commands, RenderTargetRenderPass_SetViewPort | RenderTargetRenderPass_SetScissor);

    //
    // NOTE: Draw Meshes
    //
    {
        vkCmdBindPipeline(Commands.Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, DeferredState->GBufferPipeline->Handle);
        {
            VkDescriptorSet DescriptorSets[] =
                {
                    Scene->SceneDescriptor,
                };
            vkCmdBindDescriptorSets(Commands.Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, DeferredState->GBufferPipeline->Layout, 1,
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
                vkCmdBindDescriptorSets(Commands.Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, DeferredState->GBufferPipeline->Layout, 0,
                                        ArrayCount(DescriptorSets), DescriptorSets, 0, 0);
            }
            
            VkDeviceSize Offset = 0;
            vkCmdBindVertexBuffers(Commands.Buffer, 0, 1, &CurrMesh->VertexBuffer, &Offset);
            vkCmdBindIndexBuffer(Commands.Buffer, CurrMesh->IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(Commands.Buffer, CurrMesh->NumIndices, 1, 0, 0, InstanceId);
        }
    }

    RenderTargetNextSubPass(Commands);

    //
    // NOTE: Draw point lights
    //
    {
        vkCmdBindPipeline(Commands.Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, DeferredState->PointLightPipeline->Handle);
        {
            VkDescriptorSet DescriptorSets[] =
                {
                    Scene->SceneDescriptor,
                    DeferredState->GBufferDescriptor,
                };
            vkCmdBindDescriptorSets(Commands.Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, DeferredState->PointLightPipeline->Layout, 1,
                                    ArrayCount(DescriptorSets), DescriptorSets, 0, 0);
        }
            
        VkDeviceSize Offset = 0;
        vkCmdBindVertexBuffers(Commands.Buffer, 0, 1, &DeferredState->SphereMesh->VertexBuffer, &Offset);
        vkCmdBindIndexBuffer(Commands.Buffer, DeferredState->SphereMesh->IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(Commands.Buffer, DeferredState->SphereMesh->NumIndices, Scene->NumPointLights, 0, 0, 0);
    }

    //
    // NOTE: Draw directional light
    //
    {
        vkCmdBindPipeline(Commands.Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, DeferredState->DirectionalLightPipeline->Handle);
        {
            VkDescriptorSet DescriptorSets[] =
                {
                    Scene->SceneDescriptor,
                    DeferredState->GBufferDescriptor,
                };
            vkCmdBindDescriptorSets(Commands.Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, DeferredState->DirectionalLightPipeline->Layout, 1,
                                    ArrayCount(DescriptorSets), DescriptorSets, 0, 0);
        }
            
        VkDeviceSize Offset = 0;
        vkCmdBindVertexBuffers(Commands.Buffer, 0, 1, &DeferredState->QuadMesh->VertexBuffer, &Offset);
        vkCmdBindIndexBuffer(Commands.Buffer, DeferredState->QuadMesh->IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(Commands.Buffer, DeferredState->QuadMesh->NumIndices, 1, 0, 0, 0);
    }
    
    RenderTargetRenderPassEnd(Commands);        
}
