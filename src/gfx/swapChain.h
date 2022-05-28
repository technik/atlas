#pragma once
#include <Windows.h>

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>

#include <cstdint>
#include <memory>
#include <gfx/CommandQueue.h>

class Window;

class SwapChain
{
public:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	static SwapChain* CreateSwapChain(
		Window* nativeWindow,
		uint32_t width, uint32_t height, uint32_t bufferCount);

    void setVSync(bool enable) { m_vSyncEnabled = enable; }

	ComPtr<IDXGISwapChain4> m_dxgiSwapChain;
    void Resize(uint32_t width, uint32_t height);

    // The swap chain has internal access to the graphics queue, and will signal the queue right after presenting.
    // It will then store the signal value and update the back buffer index to match the next back buffer.
    // That way you can call backBufferFence() right after present to wait on the next buffer instead of the one you just presented
    void Present();

    auto backBuffer() const { return m_BackBuffers[m_CurrentBackBufferIndex]; }
    auto backBufferIndex() const { return m_CurrentBackBufferIndex; }
    auto backBufferView() const
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(
            m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            m_CurrentBackBufferIndex,
            m_rtvDescSize);
        return rtv;
    }
    auto backBufferFence() const { return m_PresentFences[m_CurrentBackBufferIndex]; }

private:
	SwapChain(ComPtr<IDXGISwapChain4> dxgiSwapChain4, uint32_t bufferCount);
    void UpdateRenderTargetViews();

    std::vector<ComPtr<ID3D12Resource>> m_BackBuffers;
    std::vector<gfx::AwaitableFence> m_PresentFences;
	void updateHDRSupport();
	bool m_supportHDR = false;
    bool m_vSyncEnabled = false;
    UINT m_rtvDescSize;
    ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
    UINT m_CurrentBackBufferIndex;
};
