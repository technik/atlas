#pragma once

#include <cstdint>

#include "DX12Context.h"
#include "CommandQueue.h"
#include <math/matrix.h>

class RasterPipeline;
class ComputePipeline;

namespace gfx {

    class Renderer
    {
    public:
        Renderer(uint32_t rtWidth, uint32_t rtHeight);
        void BeginRender(
            ID3D12Resource* backBuffer,
            CD3DX12_CPU_DESCRIPTOR_HANDLE colorRTV,
            gfx::AwaitableFence backBufferFence);

        void EndRender(ID3D12Resource* backBuffer);

        void OnWindowResize(uint32_t width, uint32_t height);

    private:
        void InitRenderGraph(uint32_t rtWidth, uint32_t rtHeight);
        void InitRenderPipelines();
        void ResizeDepthBuffer(uint32_t w, uint32_t h);

        ComPtr<ID3D12RootSignature> m_rootSignature;
        std::shared_ptr<RasterPipeline> m_rasterPipeline;

        std::shared_ptr<ComputePipeline> m_computePipeline;

        // Render graph
        ComPtr<ID3D12Resource> m_depthBuffer;
        ComPtr<ID3D12DescriptorHeap> m_depthBufferDescHeap;

        D3D12_VIEWPORT m_viewport;
        D3D12_RECT m_scissor;
    };
}