#pragma once

#include "DX12Context.h"
#include "dxHelpers.h"

namespace gfx
{
    // Note: Just as in Vulkan, gfx command lists supersede compute command lists, which in turn supersede copy command lists.
    class CommandList
    {
    public:
        CommandList(D3D12_COMMAND_LIST_TYPE type)
        {
            m_cmdAllocator = RenderContext()->CreateCommandAllocator(type);
            m_dx12CommandList = RenderContext()->CreateCommandList(m_cmdAllocator, type);
        }

        ~CommandList()
        {
            m_cmdAllocator.Reset();
            m_dx12CommandList = nullptr;
            m_cmdAllocator = nullptr;
        }

        void Reset()
        {
            ThrowIfFailed(m_cmdAllocator->Reset());
            ThrowIfFailed(m_dx12CommandList->Reset(m_cmdAllocator.Get(), nullptr));

            // Release in flight resources
            m_inFlightResources.clear();
        }

        void ResourceBarrier(
            ComPtr<ID3D12Resource> resource,
            D3D12_RESOURCE_STATES beforeState,
            D3D12_RESOURCE_STATES afterState)
        {
            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                resource.Get(),
                beforeState, afterState);

            m_dx12CommandList->ResourceBarrier(1, &barrier);
        }

        void ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle, float color[4])
        {
            m_dx12CommandList->ClearRenderTargetView(rtv_handle, color, 0, nullptr);
            // TODO: Store the resource so that it can be released after the command list is executed
        }

        void ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle, float depth)
        {
            m_dx12CommandList->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
            // TODO: Store the resource so that it can be released after the command list is executed
        }

        UINT64 UpdateSubresources(
            ComPtr<ID3D12Resource> pDestinationResource,
            ComPtr<ID3D12Resource> pIntermediate,
            UINT64 IntermediateOffset,
            UINT FirstSubresource,
            UINT NumSubresources,
            const D3D12_SUBRESOURCE_DATA* pSrcData)
        {
            // Keep resources alive until the end of execution
            m_inFlightResources.push_back(pDestinationResource);
            m_inFlightResources.push_back(pIntermediate);

            return ::UpdateSubresources(
                m_dx12CommandList.Get(),
                pDestinationResource.Get(),
                pIntermediate.Get(),
                IntermediateOffset,
                FirstSubresource,
                NumSubresources,
                pSrcData);
        }

        ComPtr<ID3D12GraphicsCommandList> m_dx12CommandList;

    private:
        ComPtr<ID3D12CommandAllocator> m_cmdAllocator;
        std::vector<ComPtr<ID3D12Resource>> m_inFlightResources;
    };
} // namespace gfx