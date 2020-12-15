
//
// NOTE: Dx12 helpers
//

inline void Dx12CheckResult(HRESULT hr)
{
    if (FAILED(hr))
    {
        InvalidCodePath;
    }
}

//
// NOTE: Memory Arena
//

inline ID3D12Heap* Dx12HeapCreate(ID3D12Device* Device, u64 Size, D3D12_HEAP_FLAGS HeapFlags)
{
    ID3D12Heap* Result = 0;
    
    D3D12_HEAP_DESC HeapDesc = {};
    HeapDesc.SizeInBytes = Size;
    HeapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapDesc.Properties.CreationNodeMask = 0;
    HeapDesc.Properties.VisibleNodeMask = 0;
    HeapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    HeapDesc.Flags = HeapFlags;
    Dx12CheckResult(Device->CreateHeap(&HeapDesc, _uuidof(Result), (void**)&Result));

    return Result;
}

inline dx12_gpu_linear_arena Dx12GpuLinearArenaCreate(ID3D12Heap* Heap, u64 Size)
{
    dx12_gpu_linear_arena Result = {};
    Result.Size = Size;
    Result.GpuHeap = Heap;
    Result.MemPtr = 0;

    return Result;
}

inline u64 Dx12PushSize(dx12_gpu_linear_arena* Arena, u64 Size, u64 Alignment)
{
    u64 AlignedAddress = AlignAddress(Arena->Used, Alignment);
    Assert(AlignedAddress + Size <= Arena->Size);
    
    u64 Result = {};
    Result = AlignedAddress;

    Arena->Used = AlignedAddress + Size;

    return Result;
}

inline dx12_gpu_temp_mem Dx12BeginTempMem(dx12_gpu_linear_arena* Arena)
{
    // NOTE: This function lets us take all memory allocated past this point and later
    // free it
    dx12_gpu_temp_mem TempMem = {};
    TempMem.Arena = Arena;
    TempMem.Used = Arena->Used;

    return TempMem;
}

inline void Dx12EndTempMem(dx12_gpu_temp_mem TempMem)
{
    TempMem.Arena->Used = TempMem.Used;
}

//
// NOTE: Fence
//

inline dx12_fence Dx12FenceCreate(ID3D12Device* Device)
{
    dx12_fence Result = {};
    Result.CpuHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    Dx12CheckResult(Device->CreateFence(Result.Val, D3D12_FENCE_FLAG_NONE, _uuidof(ID3D12Fence), (void**)&Result.GpuHandle));

    return Result;
}

inline void Dx12SignalFence(ID3D12CommandQueue* Queue, dx12_fence* Fence)
{
    Fence->Val += 1;
    Dx12CheckResult(Queue->Signal(Fence->GpuHandle, Fence->Val));
}

inline void Dx12WaitOnFence(dx12_fence* Fence)
{
    if (Fence->GpuHandle->GetCompletedValue() <= Fence->Val)
    {
        Dx12CheckResult(Fence->GpuHandle->SetEventOnCompletion(Fence->Val, Fence->CpuHandle));
        WaitForSingleObject(Fence->CpuHandle, INFINITE);
    }
}

inline void Dx12SignalAndWait(ID3D12CommandQueue* Queue, dx12_fence* Fence)
{
    Dx12SignalFence(Queue, Fence);
    Dx12WaitOnFence(Fence);
}

//
// NOTE: Command Helpers
//

inline dx12_commands Dx12CommandsCreate(ID3D12Device* Device, ID3D12CommandAllocator* Allocator)
{
    dx12_commands Result = {};

    Dx12CheckResult(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, Allocator, 0, IID_PPV_ARGS(&Result.List)));
    Dx12CheckResult(Result.List->Close());
    Result.Fence = Dx12FenceCreate(Device);

    return Result;
}

inline void Dx12CommandsBegin(ID3D12Device* Device, ID3D12CommandAllocator* Allocator, dx12_commands* Commands, b32 ClearAllocator)
{
    Dx12WaitOnFence(&Commands->Fence);
    if (ClearAllocator)
    {
        Dx12CheckResult(Allocator->Reset());
    }
    Dx12CheckResult(Commands->List->Reset(Allocator, NULL));
}

inline void Dx12CommandsSubmit(ID3D12CommandQueue* Queue, dx12_commands* Commands)
{
    Dx12CheckResult(Commands->List->Close());
    Queue->ExecuteCommandLists(1, (ID3D12CommandList**)&Commands->List);
    Dx12SignalFence(Queue, &Commands->Fence);
}

//
// NOTE: Shader Helpers
//

// TODO: Add DXC and FXC options
inline ID3DBlob* Dx12CreateShader(linear_arena* TempArena, u32 ShaderType, char* FileName, char* MainFuncName)
{
    ID3DBlob* Result = 0;
    
    temp_mem TempMem = BeginTempMem(TempArena);
    
    FILE* File = fopen(FileName, "rb");
    if (!File)
    {
        InvalidCodePath;
    }

    fseek(File, 0, SEEK_END);
    u32 CodeSize = ftell(File);
    fseek(File, 0, SEEK_SET);
    u32* Code = (u32*)PushSize(TempArena, CodeSize);
    fread(Code, CodeSize, 1, File);
    fclose(File);

    char* ShaderTypeStr = 0;
    switch (ShaderType)
    {
        case ShaderType_Vertex:
        {
            ShaderTypeStr = "vs_5_0";
        } break;

        case ShaderType_Fragment:
        {
            ShaderTypeStr = "ps_5_0";
        } break;

        case ShaderType_Compute:
        {
            ShaderTypeStr = "cs_5_0";
        } break;

        default:
        {
            InvalidCodePath;
        } break;
    }
    
    ID3DBlob* ErrorMsg = 0;
    HRESULT VsResult = D3DCompile(Code, CodeSize, NULL, NULL, 0, MainFuncName, ShaderTypeStr, 0, 0, &Result, &ErrorMsg);
    if (VsResult != S_OK)
    {
        OutputDebugStringA((LPCSTR)ErrorMsg->GetBufferPointer());
        InvalidCodePath;
    }

    if (ErrorMsg)
    {
        ErrorMsg->Release();
    }

    EndTempMem(TempMem);
    
    return Result;
}

//
// NOTE: Buffer Helpers
//

inline void Dx12BufferCreate(ID3D12Device* Device, dx12_gpu_linear_arena* Arena, D3D12_RESOURCE_FLAGS Flags, u64 BufferSize,
                             ID3D12Resource** OutBuffer, u64* OutGpuPtr)
{
    D3D12_RESOURCE_DESC ResourceDesc = {};
    ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    ResourceDesc.Alignment = 0;
    ResourceDesc.Width = BufferSize;
    ResourceDesc.Height = 1;
    ResourceDesc.DepthOrArraySize = 1;
    ResourceDesc.MipLevels = 1;
    ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    ResourceDesc.SampleDesc.Count = 1;
    ResourceDesc.SampleDesc.Quality = 0;
    ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ResourceDesc.Flags = Flags;

    *OutGpuPtr = Dx12PushSize(Arena, BufferSize, KiloBytes(64));
    Dx12CheckResult(Device->CreatePlacedResource(Arena->GpuHeap, *OutGpuPtr, &ResourceDesc, D3D12_RESOURCE_STATE_COMMON, NULL,
                                                 _uuidof(ID3D12Resource), (void**)OutBuffer));
}

inline ID3D12Resource* Dx12BufferCreate(ID3D12Device* Device, dx12_gpu_linear_arena* Arena, D3D12_RESOURCE_FLAGS Flags, u64 BufferSize)
{
    ID3D12Resource* Result = 0;
    u64 GpuPtr = 0;
    Dx12BufferCreate(Device, Arena, Flags, BufferSize, &Result, &GpuPtr);
    return Result;
}

//
// NOTE: Image Helpers
//

//
// NOTE: Pipeline Helpers
//

//
// NOTE: FrameBuffer Helpers
//

//
// NOTE: Root Signature Helpers
//

//
// NOTE: Descriptor Heap Helpers
//

inline dx12_descriptor_heap_builder Dx12DescHeapBuilderCreate(ID3D12Device* Device, ID3D12DescriptorHeap* DescHeap,
                                                              D3D12_DESCRIPTOR_HEAP_TYPE Type, u32 MaxNumDescriptors)
{
    dx12_descriptor_heap_builder Result = {};
    Result.DescHeap = DescHeap;
    Result.DsIncrement = Device->GetDescriptorHandleIncrementSize(Type);
    Result.MaxNumDescriptors = MaxNumDescriptors;

    return Result;
}

inline void Dx12AllocateDescirptor(dx12_descriptor_heap_builder* DescHeapBuilder, D3D12_CPU_DESCRIPTOR_HANDLE* OutCpuDesc,
                                   D3D12_GPU_DESCRIPTOR_HANDLE* OutGpuDesc)
{
    Assert(DescHeapBuilder->CurrDescId < DescHeapBuilder->MaxNumDescriptors);
    
    D3D12_CPU_DESCRIPTOR_HANDLE CpuDesc = DescHeapBuilder->DescHeap->GetCPUDescriptorHandleForHeapStart();
    CpuDesc.ptr += DescHeapBuilder->DsIncrement*DescHeapBuilder->CurrDescId;

    D3D12_GPU_DESCRIPTOR_HANDLE GpuDesc = DescHeapBuilder->DescHeap->GetGPUDescriptorHandleForHeapStart();
    GpuDesc.ptr += DescHeapBuilder->DsIncrement*DescHeapBuilder->CurrDescId;

    DescHeapBuilder->CurrDescId += 1;

    *OutCpuDesc = CpuDesc;
    *OutGpuDesc = GpuDesc;
}

//
// NOTE: Barrier Manager
//

inline dx12_barrier_manager Dx12BarrierManagerCreate(linear_arena* Arena, u32 MaxNumBarriers)
{
    dx12_barrier_manager Result = {};

    Result.MaxNumBarriers = MaxNumBarriers;
    Result.NumBarriers = 0;
    Result.BarrierArray = PushArray(Arena, D3D12_RESOURCE_BARRIER, MaxNumBarriers);

    return Result;
}

inline void Dx12BarrierAdd(dx12_barrier_manager* Manager, D3D12_RESOURCE_BARRIER Barrier)
{
    Assert(Manager->NumBarriers < Manager->MaxNumBarriers);
    Manager->BarrierArray[Manager->NumBarriers++] = Barrier;
}

inline void Dx12BarrierManagerFlush(dx12_barrier_manager* Manager, ID3D12GraphicsCommandList* CommandList)
{
    CommandList->ResourceBarrier(Manager->NumBarriers, Manager->BarrierArray);
    Manager->NumBarriers = 0;
}

//
// NOTE: Transfer Manager
//

inline dx12_transfer_manager Dx12TransferManagerCreate(ID3D12Device* Device, linear_arena* CpuArena, dx12_gpu_linear_arena* GpuArena,
                                                       u64 StagingSize, u32 MaxNumBufferTransfers, u32 MaxNumImageTransfers)
{
    dx12_transfer_manager Result = {};

    u32 ArenaSize = (MaxNumBufferTransfers*sizeof(dx12_buffer_transfer) +
                     MaxNumImageTransfers*sizeof(dx12_image_transfer));
    Result.Arena = LinearSubArena(CpuArena, ArenaSize);

    // TODO: Do we need any alignment for staging?
    
    // NOTE: Staging data
    {
        Result.StagingSize = StagingSize;
        CD3DX12_HEAP_PROPERTIES HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(Result.StagingSize);
        Dx12CheckResult(Device->CreateCommittedResource(&HeapProp, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
                                                        0, IID_PPV_ARGS(&Result.StagingBuffer)));

        CD3DX12_RANGE ReadRange(0, Result.StagingSize);
        Result.StagingBuffer->Map(0, &ReadRange, (void**)&Result.StagingPtr);
    }
    
    // NOTE: Buffer data
    Result.MaxNumBufferTransfers = MaxNumBufferTransfers;
    Result.BufferTransferArray = PushArray(CpuArena, dx12_buffer_transfer, MaxNumBufferTransfers);

    // NOTE: Image data
    Result.MaxNumImageTransfers = MaxNumImageTransfers;
    Result.ImageTransferArray = PushArray(CpuArena, dx12_image_transfer, MaxNumImageTransfers);
    
    return Result;
}

#define Dx12TransferPushBufferWriteStruct(Updater, Buffer, Type, Alignment, InputMask, OutputMask) \
    (Type*)Dx12TransferPushBufferWrite(Updater, Buffer, sizeof(Type), Alignment, InputMask, OutputMask)
#define Dx12TransferPushBufferWriteArray(Updater, Buffer, Type, Count, Alignment, InputMask, OutputMask) \
    (Type*)Dx12TransferPushBufferWrite(Updater, Buffer, sizeof(Type)*Count, Alignment, InputMask, OutputMask)
inline u8* Dx12TransferPushBufferWrite(dx12_transfer_manager* Manager, ID3D12Resource* Buffer, u64 BufferSize, u64 Alignment,
                                       D3D12_RESOURCE_STATES InputState, D3D12_RESOURCE_STATES OutputState)
{
    Manager->StagingOffset = AlignAddress(Manager->StagingOffset, Alignment);
    Assert((Manager->StagingOffset + BufferSize) <= Manager->StagingSize);
    u8* Result = Manager->StagingPtr + Manager->StagingOffset;
 
    Assert(Manager->NumBufferTransfers < Manager->MaxNumBufferTransfers);
    dx12_buffer_transfer* Transfer = Manager->BufferTransferArray + Manager->NumBufferTransfers++;
    Transfer->Resource = Buffer;
    Transfer->Size = BufferSize;
    Transfer->StagingOffset = Manager->StagingOffset;
    Transfer->InputState = InputState;
    Transfer->OutputState = OutputState;

    Manager->StagingOffset += BufferSize;
    return Result;
}

inline u8* Dx12TransferPushImageWrite(dx12_transfer_manager* Manager, ID3D12Resource* Image, u32 Width, u32 Height, DXGI_FORMAT Format,
                                      D3D12_RESOURCE_STATES InputState, D3D12_RESOURCE_STATES OutputState)
{
    // IMPORTANT: The caller has to make sure to align their writes to pitch size
    // TODO: Do we need alignment?
    // TODO: Handle format sizes here?
    u8* Result = Manager->StagingPtr + Manager->StagingOffset;

    dx12_image_transfer* Transfer = Manager->ImageTransferArray + Manager->NumImageTransfers++;
    Transfer->Resource = Image;
    Transfer->InputState = InputState;
    Transfer->OutputState = OutputState;
    Transfer->PlacedTexture2d.Offset = Manager->StagingOffset;
    Transfer->PlacedTexture2d.Footprint.Format = Format;
    Transfer->PlacedTexture2d.Footprint.Width = Width;
    Transfer->PlacedTexture2d.Footprint.Height = Height;
    Transfer->PlacedTexture2d.Footprint.Depth = 1;
    // TODO: We assume that the size of our format is dword, handle format sizes?
    Transfer->PlacedTexture2d.Footprint.RowPitch = AlignAddress(Width * (u32)sizeof(DWORD), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
        
    Manager->StagingOffset += Transfer->PlacedTexture2d.Footprint.RowPitch * Height;
    Assert(Manager->StagingOffset <= Manager->StagingSize);

    return Result;
}

inline void Dx12TransferManagerFlush(dx12_transfer_manager* Manager, ID3D12Device* Device, ID3D12GraphicsCommandList* CommandList,
                                     dx12_barrier_manager* BarrierBatcher)
{
    // NOTE: Transfer all buffers
    if (Manager->NumBufferTransfers > 0)
    {
        for (u32 BufferId = 0; BufferId < Manager->NumBufferTransfers; ++BufferId)
        {
            dx12_buffer_transfer* BufferTransfer = Manager->BufferTransferArray + BufferId;
            Dx12BarrierAdd(BarrierBatcher, CD3DX12_RESOURCE_BARRIER::Transition(BufferTransfer->Resource, BufferTransfer->InputState,
                                                                                D3D12_RESOURCE_STATE_COPY_DEST));
        }
        Dx12BarrierManagerFlush(BarrierBatcher, CommandList);

        for (u32 BufferId = 0; BufferId < Manager->NumBufferTransfers; ++BufferId)
        {
            dx12_buffer_transfer* BufferTransfer = Manager->BufferTransferArray + BufferId;
            CommandList->CopyBufferRegion(BufferTransfer->Resource, 0, Manager->StagingBuffer, BufferTransfer->StagingOffset, BufferTransfer->Size);
        }
        
        for (u32 BufferId = 0; BufferId < Manager->NumBufferTransfers; ++BufferId)
        {
            dx12_buffer_transfer* BufferTransfer = Manager->BufferTransferArray + BufferId;
            Dx12BarrierAdd(BarrierBatcher, CD3DX12_RESOURCE_BARRIER::Transition(BufferTransfer->Resource, D3D12_RESOURCE_STATE_COPY_DEST,
                                                                                BufferTransfer->InputState));
        }
        Dx12BarrierManagerFlush(BarrierBatcher, CommandList);
        Manager->NumBufferTransfers = 0;
    }

    // NOTE: Transfer all images
    if (Manager->NumImageTransfers > 0)
    {
        for (u32 ImageId = 0; ImageId < Manager->NumImageTransfers; ++ImageId)
        {
            dx12_image_transfer* ImageTransfer = Manager->ImageTransferArray + ImageId;
            Dx12BarrierAdd(BarrierBatcher, CD3DX12_RESOURCE_BARRIER::Transition(ImageTransfer->Resource, ImageTransfer->InputState,
                                                                                D3D12_RESOURCE_STATE_COPY_DEST));
        }
        Dx12BarrierManagerFlush(BarrierBatcher, CommandList);

        for (u32 ImageId = 0; ImageId < Manager->NumImageTransfers; ++ImageId)
        {
            dx12_image_transfer* ImageTransfer = Manager->ImageTransferArray + ImageId;
            D3D12_TEXTURE_COPY_LOCATION SrcCopy = CD3DX12_TEXTURE_COPY_LOCATION(Manager->StagingBuffer, ImageTransfer->PlacedTexture2d);
            D3D12_TEXTURE_COPY_LOCATION DstCopy = CD3DX12_TEXTURE_COPY_LOCATION(ImageTransfer->Resource, 0);
            CommandList->CopyTextureRegion(&DstCopy, 0, 0, 0, &SrcCopy, NULL);
        }
        
        for (u32 ImageId = 0; ImageId < Manager->NumImageTransfers; ++ImageId)
        {
            dx12_image_transfer* ImageTransfer = Manager->ImageTransferArray + ImageId;
            Dx12BarrierAdd(BarrierBatcher, CD3DX12_RESOURCE_BARRIER::Transition(ImageTransfer->Resource, D3D12_RESOURCE_STATE_COPY_DEST,
                                                                                ImageTransfer->OutputState));
        }
        Dx12BarrierManagerFlush(BarrierBatcher, CommandList);
        Manager->NumImageTransfers = 0;
    }
}
