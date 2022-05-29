#include "renderer.h"

#include "dxHelpers.h"
#include "rasterPipeline.h"
#include "ComputePipeline.h"
#include "gfx/GPUAllocator.h"
#include <shaders/staticDirect.h>

using namespace math;
using namespace hlsl;

namespace gfx
{
    Renderer::Renderer(uint32_t rtWidth, uint32_t rtHeight)
    {
        InitRenderGraph(rtWidth, rtHeight);
        InitRenderPipelines();
    }

    void Renderer::BeginRender(
        ID3D12Resource* backBuffer,
        CD3DX12_CPU_DESCRIPTOR_HANDLE colorRTV,
        gfx::AwaitableFence backBufferFence)
    {
        // Get a command list for the frame
        auto& gfxQueue = gfx::RenderContext()->graphicsQueue();
        auto cmd = gfxQueue.getNewCommandList();

        // Transition the back buffer to Render Target State
        cmd->ResourceBarrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        // Clear the depth and color render targets
        auto dsv = m_depthBufferDescHeap->GetCPUDescriptorHandleForHeapStart();
        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
        cmd->ClearRenderTargetView(colorRTV, clearColor);
        cmd->ClearDepthStencilView(dsv, 0.f);

        // Set render state
        auto dx12Cmd = cmd->m_dx12CommandList;
        dx12Cmd->SetGraphicsRootSignature(m_rootSignature.Get());
        dx12Cmd->SetPipelineState(m_rasterPipeline->m_dx12Pipeline.Get());

        dx12Cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        dx12Cmd->RSSetViewports(1, &m_viewport);
        dx12Cmd->RSSetScissorRects(1, &m_scissor);
        dx12Cmd->OMSetRenderTargets(1, &colorRTV, FALSE, &dsv);

        // Finish recording
        ThrowIfFailed(dx12Cmd->Close());

        // Wait for the frame buffer to be available for rendering before submitting the command list
        gfxQueue.spinOnFence(backBufferFence);
        gfxQueue.submitCommandList(cmd);
    }

    void Renderer::EndRender(ID3D12Resource* backBuffer)
    {
        // Get a command list for the frame
        auto& gfxQueue = gfx::RenderContext()->graphicsQueue();
        auto cmd = gfxQueue.getNewCommandList();

        // Transition the back buffer to Present state
        cmd->ResourceBarrier(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        // Finish recording
        auto dx12Cmd = cmd->m_dx12CommandList;
        ThrowIfFailed(dx12Cmd->Close());
        gfxQueue.submitCommandList(cmd);
    }

    void Renderer::OnWindowResize(uint32_t width, uint32_t height)
    {
        m_scissor = CD3DX12_RECT(0, 0, width, height);
        m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
        ResizeDepthBuffer(width, height);
    }

    void Renderer::InitRenderGraph(uint32_t rtWidth, uint32_t rtHeight)
    {
        m_scissor = CD3DX12_RECT(0, 0, rtWidth, rtHeight);
        m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(rtWidth), static_cast<float>(rtHeight));
        auto device = gfx::RenderContext()->m_dx12Device;

        // Depth buffer
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE ?
        ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_depthBufferDescHeap)));

        ResizeDepthBuffer(rtWidth, rtHeight);
    }

    void Renderer::InitRenderPipelines()
    {
        // Create a root signature.
        // Allow input layout and deny unnecessary access to certain pipeline stages.
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        // Root constants
        CD3DX12_ROOT_PARAMETER1 rootParameters[2];
        // Instance data
        rootParameters[0].InitAsConstants(sizeof(InstanceData) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        rootParameters[1].InitAsConstants(sizeof(FrameData) / 4, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX);

        // Root signature description
        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

        // Serialize the root signature.
        ComPtr<ID3DBlob> rootSignatureBlob;
        ComPtr<ID3DBlob> errorBlob;
        ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc,
            D3D_ROOT_SIGNATURE_VERSION_1_1, &rootSignatureBlob, &errorBlob));
        // Create the root signature.
        auto device = gfx::RenderContext()->m_dx12Device;
        ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
            rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

        // Load raster pipeline
        m_rasterPipeline = std::make_shared<RasterPipeline>(
            m_rootSignature,
            "staticDirect.vshader.cso",
            "staticDirect.pshader.cso",
            true);
    }

    void Renderer::ResizeDepthBuffer(uint32_t w, uint32_t h)
    {
        if (!w || !h)
            return;
        gfx::RenderContext()->graphicsQueue().flush();
        auto device = gfx::RenderContext()->m_dx12Device;
        auto& alloc = gfx::RenderContext()->allocator();

        m_depthBuffer = alloc.CreateDepthBuffer(w, h, 0.f);

        // Update the depth-stencil view.
        D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
        dsv.Format = DXGI_FORMAT_D32_FLOAT;
        dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsv.Texture2D.MipSlice = 0;
        dsv.Flags = D3D12_DSV_FLAG_NONE;

        device->CreateDepthStencilView(m_depthBuffer.Get(), &dsv,
            m_depthBufferDescHeap->GetCPUDescriptorHandleForHeapStart());
    }
}