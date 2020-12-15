#pragma once

//
// NOTE: Memory
//

struct dx12_gpu_linear_arena
{
    u64 Size;
    u64 Used;
    ID3D12Heap* GpuHeap;
    u64 MemPtr;
};

struct dx12_gpu_temp_mem
{
    dx12_gpu_linear_arena* Arena;
    u64 Used;
};

//
// NOTE: Commands
//

struct dx12_fence
{
    HANDLE CpuHandle;
    ID3D12Fence* GpuHandle;
    UINT64 Val;
};

struct dx12_commands
{
    ID3D12GraphicsCommandList* List;
    dx12_fence Fence;
};

//
// NOTE: Descriptor Heap Builder
//

struct dx12_descriptor_heap_builder
{
    ID3D12DescriptorHeap* DescHeap;
    u32 MaxNumDescriptors;
    u32 CurrDescId;
    UINT DsIncrement;
};

//
// NOTE: Barrier Batcher
//

struct dx12_barrier_manager
{
    u32 MaxNumBarriers;
    u32 NumBarriers;
    D3D12_RESOURCE_BARRIER* BarrierArray;
};

//
// NOTE: Transfer Manager
//

struct dx12_buffer_transfer
{
    ID3D12Resource* Resource;
    u64 StagingOffset;
    u64 Size;

    D3D12_RESOURCE_STATES InputState;
    D3D12_RESOURCE_STATES OutputState;
};

struct dx12_image_transfer
{
    ID3D12Resource* Resource;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedTexture2d;

    D3D12_RESOURCE_STATES InputState;
    D3D12_RESOURCE_STATES OutputState;
};

struct dx12_transfer_manager
{
    linear_arena Arena;

    // NOTE: Staging data
    u64 StagingSize;
    u64 StagingOffset;
    u8* StagingPtr;
    ID3D12Resource* StagingBuffer;
    
    // NOTE: Buffer data
    u32 MaxNumBufferTransfers;
    u32 NumBufferTransfers;
    dx12_buffer_transfer* BufferTransferArray;

    // NOTE: Image data
    u32 MaxNumImageTransfers;
    u32 NumImageTransfers;
    dx12_image_transfer* ImageTransferArray;
};

#include "dx12_utils.cpp"
