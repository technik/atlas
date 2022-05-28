
#include "GPUAllocator.h"

namespace gfx
{
    GpuBuffer GPUAllocator::CreateResourceWithDataInternal(
        CommandList& commandList,
        size_t size, const void* bufferData,
        D3D12_RESOURCE_FLAGS flags)
    {
        auto device = gfx::RenderContext()->m_dx12Device;

        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto resourceDescBuffer = CD3DX12_RESOURCE_DESC::Buffer(size, flags);

        GpuBuffer result;
        result.byteCount = size;
        ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDescBuffer,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&result.gpuBuffer)));

        // Create an committed resource for the upload.
        if (bufferData)
        {
            auto stagingBuffer = GetStagingBuffer(size);

            D3D12_SUBRESOURCE_DATA subresourceData = {};
            subresourceData.pData = bufferData;
            subresourceData.RowPitch = size;
            subresourceData.SlicePitch = subresourceData.RowPitch;

            commandList.UpdateSubresources(
                result.gpuBuffer, stagingBuffer.dx12Resource,
                0, 0, 1, &subresourceData);

            // TODO: Return staging buffer to the heap when execution finishes
            // commandList.OnExecutionFinished([this, stagingBuffer]() {
            //    m_freeStagingBuffers.push_back(stagingBuffer);
            //    });
        }

        return result;
    }

    ComPtr<ID3D12Resource> GPUAllocator::CreateDepthBuffer(uint32_t w, uint32_t h, float optimalClearDepth)
    {
        auto device = gfx::RenderContext()->m_dx12Device;

        // Create a depth buffer.
        D3D12_CLEAR_VALUE optimizedClearValue = {};
        optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        optimizedClearValue.DepthStencil = { optimalClearDepth, 0 };

        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, w, h,
            1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        ComPtr<ID3D12Resource> result;
        ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &optimizedClearValue,
            IID_PPV_ARGS(&result)
        ));

        return result;
    }

    void* GPUAllocator::GetUploadBuffer(size_t size)
    {
        // Get a staging buffer
        auto stagingBuffer = GetStagingBuffer(size);
        if (!stagingBuffer.dx12Resource)
            return nullptr;

        // Map internal memory
        D3D12_RANGE memoryRange = {};
        memoryRange.Begin = 0;
        memoryRange.End = size;
        void* mappedMemory;
        stagingBuffer.dx12Resource->Map(0, &memoryRange, &mappedMemory);

        // Register mapped memory
        m_mappedBuffers.insert({ mappedMemory, stagingBuffer });

        return mappedMemory;
    }

    ComPtr<ID3D12Resource> GPUAllocator::SendToGPU(CommandList& cmdList, void* buffer)
    {
        // Locate the  corresponding staging buffer
        auto iter = m_mappedBuffers.find(buffer);
        assert(m_mappedBuffers.end() != iter);

        assert(false && "Not implemented");

        return nullptr;
    }

    GPUBuffer GPUAllocator::GetStagingBuffer(size_t size)
    {
        for (int i = 0; i < m_freeStagingBuffers.size(); ++i)
        {
            auto& buffer = m_freeStagingBuffers[i];
            if (buffer.size < size)
                continue;

            std::swap(buffer, m_freeStagingBuffers.back());
            auto result = m_freeStagingBuffers.back();
            m_freeStagingBuffers.pop_back();
            return result;
        }

        // No appropriate buffer found. Allocate a new one.
        const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        const auto resourceDescBuffer = CD3DX12_RESOURCE_DESC::Buffer(size);

        GPUBuffer result;
        result.size = size;

        auto device = gfx::RenderContext()->m_dx12Device;
        ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDescBuffer,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&result.dx12Resource)));

        return result;
    }
}