#pragma once

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// D3D12 extension library.
#include <d3dx12.h>
#include <chrono>
#include <utility>
#include <vector>

template<class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

namespace gfx {

	class CommandQueue;
	class GPUAllocator;

	class DX12Context
	{
	public:
		static void Init() { sInstance = new DX12Context; }
		static void Deinit() { delete sInstance; sInstance = nullptr; }
		static DX12Context* Get() { return sInstance; }

		ComPtr<ID3D12Device2> m_dx12Device;
		ComPtr<IDXGIFactory6> m_dxgiFactory;
		ComPtr<IDXGIAdapter1> m_adapter; // Vulkan's physical device

		auto& graphicsQueue() const { return *m_graphicsQueue; }
		auto& computeQueue() const { return *m_computeQueue; }
		auto& copyQueue() const { return *m_copyQueue; }
		GPUAllocator& allocator() const { return *m_allocator; }

		typedef std::pair<ComPtr<IDXGIOutput>, DXGI_OUTPUT_DESC> OutputDevice;
		typedef std::pair<ComPtr<IDXGIAdapter1>, std::vector<OutputDevice>> AdapterDevice;
		std::vector<AdapterDevice>      m_AdapterList;

		ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);

		ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type);

		ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type);

		ComPtr<ID3D12Fence> CreateFence();

		// Feature support
		bool m_FreeSyncSupported = false;
		bool m_DXRSupported = false;

	private:

		DX12Context();
		~DX12Context(); // Out of line destructor to allow forward declaration of CommandQueue

		static ComPtr<IDXGIFactory6> createDXGIFactory();
		void checkSupportedFeatures();
		bool checkFreeSyncSupport();
		ComPtr<IDXGIAdapter1> getAdapter();
		ComPtr<ID3D12Device2> CreateDevice();

		std::unique_ptr<CommandQueue> m_graphicsQueue;
		std::unique_ptr<CommandQueue> m_computeQueue;
		std::unique_ptr<CommandQueue> m_copyQueue;
		GPUAllocator* m_allocator = nullptr;

		static inline DX12Context* sInstance = nullptr;
	};

	inline auto RenderContext()
	{
		return DX12Context::Get();
	}
} // namespace gfx