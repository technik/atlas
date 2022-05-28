#include "swapChain.h"

#include "dxHelpers.h"
#include "DX12Context.h"
#include "Window.h"
#include "gfx/CommandQueue.h"

SwapChain* SwapChain::CreateSwapChain(
	Window* nativeWindow,
	uint32_t width, uint32_t height, uint32_t bufferCount)
{
	ComPtr<IDXGISwapChain4> dxgiSwapChain4;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = bufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // Discard frame contents after Present().
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	// It is recommended to always allow tearing if tearing support is available.
	swapChainDesc.Flags = gfx::RenderContext()->m_FreeSyncSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	ComPtr<IDXGISwapChain1> swapChain1;
	auto dxgiFactory = gfx::RenderContext()->m_dxgiFactory;
	auto* gfxQueue = gfx::RenderContext()->graphicsQueue().getNativeQueue().Get();
	ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
		gfxQueue,
		nativeWindow->m_hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1));

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
	// will be handled manually.
	ThrowIfFailed(dxgiFactory->MakeWindowAssociation(nativeWindow->m_hWnd, DXGI_MWA_NO_ALT_ENTER));
	//ThrowIfFailed(dxgiFactory->MakeWindowAssociation(nativeWindow->m_hWnd, 0));

	ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

	auto swapChain = new SwapChain(dxgiSwapChain4, bufferCount);

	return swapChain;
}

SwapChain::SwapChain(ComPtr<IDXGISwapChain4> dxgiSwapChain4, uint32_t bufferCount)
	: m_dxgiSwapChain(dxgiSwapChain4)
{
    m_rtvDescriptorHeap = gfx::RenderContext()->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, bufferCount);
    m_rtvDescSize = gfx::RenderContext()->m_dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    m_CurrentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();
	updateHDRSupport();
    m_BackBuffers.resize(bufferCount);
	m_PresentFences.resize(bufferCount);

    UpdateRenderTargetViews();
}

void SwapChain::Present()
{
    UINT syncInterval = m_vSyncEnabled ? 1 : 0;
    UINT presentFlags = (gfx::RenderContext()->m_FreeSyncSupported && !m_vSyncEnabled) ? DXGI_PRESENT_ALLOW_TEARING : 0;
    ThrowIfFailed(m_dxgiSwapChain->Present(syncInterval, presentFlags));

	m_PresentFences[m_CurrentBackBufferIndex] = gfx::RenderContext()->graphicsQueue().signal();

    m_CurrentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();
}

void SwapChain::updateHDRSupport()
{
	m_supportHDR = false;
	bool hdrAvailable = false;

	if (m_dxgiSwapChain != nullptr)
	{
		// Get information about the display we are presenting to.
		ComPtr<IDXGIOutput> output;
		ComPtr<IDXGIOutput6> output6;
		if (SUCCEEDED(m_dxgiSwapChain->GetContainingOutput(output.ReleaseAndGetAddressOf())))
		{
			output.As(&output6);
			if (output6 != nullptr)
			{
				DXGI_OUTPUT_DESC1 outputDesc;
				output6->GetDesc1(&outputDesc);
				// If the display supports HDR, then we'll enable high color.
				m_supportHDR = outputDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
			}
		}
	}
}

void SwapChain::UpdateRenderTargetViews()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    for (int i = 0; i < m_BackBuffers.size(); ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        ThrowIfFailed(m_dxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		gfx::RenderContext()->m_dx12Device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

        m_BackBuffers[i] = backBuffer;

        rtvHandle.Offset(m_rtvDescSize);
    }
}

void SwapChain::Resize(uint32_t width, uint32_t height)
{
    for (int i = 0; i < m_BackBuffers.size(); ++i)
    {
        // Any references to the back buffers must be released
        // before the swap chain can be resized.
        m_BackBuffers[i].Reset();
        //g_FrameFenceValues[i] = g_FrameFenceValues[g_CurrentBackBufferIndex];
    }
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    ThrowIfFailed(m_dxgiSwapChain->GetDesc(&swapChainDesc));
    ThrowIfFailed(m_dxgiSwapChain->ResizeBuffers(m_BackBuffers.size(), width, height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

    UpdateRenderTargetViews();

    m_CurrentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();
}