#pragma once

#include <unordered_map>

#include "DX12Context.h"
#include "dxHelpers.h"
#include "CommandQueue.h"
#include "GPUResource.h"

namespace gfx
{
    struct GpuBuffer
    {
        D3D12_VERTEX_BUFFER_VIEW AsVtxBufferView(size_t offset, size_t size, size_t stride) const
        {
            D3D12_VERTEX_BUFFER_VIEW view;
            view.BufferLocation = gpuBuffer->GetGPUVirtualAddress() + offset;
            view.SizeInBytes = size;
            view.StrideInBytes = stride;
            return view;
        }

        D3D12_VERTEX_BUFFER_VIEW AsVtxBufferView(size_t stride) const
        {
            return AsVtxBufferView(0, byteCount, stride);
        }

        D3D12_INDEX_BUFFER_VIEW AsIndexBufferView(size_t offset, size_t size, DXGI_FORMAT format) const
        {
            D3D12_INDEX_BUFFER_VIEW view;
            view.BufferLocation = gpuBuffer->GetGPUVirtualAddress() + offset;
            view.SizeInBytes = size;
            view.Format = format;
            return view;
        }

        D3D12_INDEX_BUFFER_VIEW AsIndexBufferView(DXGI_FORMAT format) const
        {
            return AsIndexBufferView(0, byteCount, format);
        }

        ComPtr<ID3D12Resource> gpuBuffer;
        size_t byteCount;
    };

    class GPUAllocator
    {
    public:
        void* GetUploadBuffer(size_t size);
        ComPtr<ID3D12Resource> SendToGPU(CommandList& cmdList, void* buffer);

        template<class T>
        GpuBuffer CreateResourceWithData(
            CommandList& commandList,
            size_t count, const T* data,
            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE)
        {
            return CreateResourceWithDataInternal(commandList, sizeof(T) * count, data, flags);
        }

        ComPtr<ID3D12Resource> CreateDepthBuffer(uint32_t w, uint32_t h, float optimalClearDepth);

    private:
        GpuBuffer CreateResourceWithDataInternal(
            CommandList& commandList,
            size_t size, const void* bufferData,
            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

        GPUBuffer GetStagingBuffer(size_t size);

        // buffer pool
        std::vector<GPUBuffer> m_freeStagingBuffers;
        // Mapped memory
        std::unordered_map<void*, GPUBuffer> m_mappedBuffers;
        // In flight stating buffers
        std::vector<GPUBuffer> m_inFlightStagingBuffers;
    };
}