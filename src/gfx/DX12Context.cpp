#include <Windows.h>
#include "DX12Context.h"
#include "dxHelpers.h"

#include <gfx/CommandQueue.h>
#include <gfx/GPUAllocator.h>

namespace WRL = Microsoft::WRL;

namespace
{
	void EnableDebugLayer()
	{
#if defined(_DEBUG)
		// Always enable the debug layer before doing anything DX12 related
		// so all possible errors generated while creating DX12 objects
		// are caught by the debug layer.
		Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
		debugInterface->EnableDebugLayer();
#endif
	}
}

namespace gfx {

	DX12Context::DX12Context()
	{
		EnableDebugLayer();

		m_dxgiFactory = createDXGIFactory();
		checkSupportedFeatures();
		m_adapter = getAdapter();
		m_dx12Device = CreateDevice();

		// Init queues
		m_copyQueue = std::make_unique<CommandQueue>(D3D12_COMMAND_LIST_TYPE_COPY, *m_dx12Device.Get(), L"Copy Queue");
		m_computeQueue = std::make_unique<CommandQueue>(D3D12_COMMAND_LIST_TYPE_COMPUTE, *m_dx12Device.Get(), L"Compute Queue");
		m_graphicsQueue = std::make_unique<CommandQueue>(D3D12_COMMAND_LIST_TYPE_DIRECT, *m_dx12Device.Get(), L"Graphics Queue");
		m_allocator = new GPUAllocator();
	}

	DX12Context::~DX12Context()
	{
		// Ensure we delete things in the oposite order they were initialized
		m_graphicsQueue = nullptr;
		m_computeQueue = nullptr;
		m_copyQueue = nullptr;
		m_dx12Device = nullptr;
		m_adapter = nullptr;
		m_dxgiFactory = nullptr;
	}

	auto DX12Context::createDXGIFactory() -> ComPtr<IDXGIFactory6>
	{
		ComPtr<IDXGIFactory6> dxgiFactory6;
		UINT createFactoryFlags = 0;
#if defined(_DEBUG)
		createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

		ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory6)));

		return dxgiFactory6;
	}

	void DX12Context::checkSupportedFeatures()
	{
		m_FreeSyncSupported = checkFreeSyncSupport();
	}

	bool DX12Context::checkFreeSyncSupport()
	{
		// Check free sync support
		bool allowTearing = false;
		auto res = m_dxgiFactory->CheckFeatureSupport(
			DXGI_FEATURE_PRESENT_ALLOW_TEARING,
			&allowTearing, sizeof(allowTearing));

		if (!SUCCEEDED(res))
		{
			allowTearing = false;
		}

		return allowTearing;
	}

	auto DX12Context::getAdapter() -> ComPtr<IDXGIAdapter1>
	{
		ComPtr<IDXGIFactory4> dxgiFactory;
		UINT createFactoryFlags = 0;
#if defined(_DEBUG)
		createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

		ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));
		ComPtr<IDXGIAdapter1> dxgiAdapter1;
		ComPtr<IDXGIAdapter4> dxgiAdapter4;

		SIZE_T maxDedicatedVideoMemory = 0;
		for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

			// Check to see if the adapter can create a D3D12 device without actually 
			// creating it. The adapter with the largest dedicated video memory
			// is favored.
			if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),
					D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
				dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
			{
				maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
			}
		}

		return dxgiAdapter1;
	}

	auto DX12Context::CreateDevice() -> ComPtr<ID3D12Device2>
	{
		ComPtr<ID3D12Device2> d3d12Device2;
		ThrowIfFailed(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&d3d12Device2)));

#if defined(_DEBUG)
		ComPtr<ID3D12InfoQueue> pInfoQueue;
		if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
		{
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
			// Suppress whole categories of messages
			//D3D12_MESSAGE_CATEGORY Categories[] = {};

			// Suppress messages based on their severity level
			D3D12_MESSAGE_SEVERITY Severities[] =
			{
				D3D12_MESSAGE_SEVERITY_INFO
			};

			// Suppress individual messages by their ID
			D3D12_MESSAGE_ID DenyIds[] = {
				D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
				D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
				D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
			};

			D3D12_INFO_QUEUE_FILTER NewFilter = {};
			//NewFilter.DenyList.NumCategories = _countof(Categories);
			//NewFilter.DenyList.pCategoryList = Categories;
			NewFilter.DenyList.NumSeverities = _countof(Severities);
			NewFilter.DenyList.pSeverityList = Severities;
			NewFilter.DenyList.NumIDs = _countof(DenyIds);
			NewFilter.DenyList.pIDList = DenyIds;

			ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
		}
#endif

		return d3d12Device2;
	}

	WRL::ComPtr<ID3D12DescriptorHeap> DX12Context::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
	{
		WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;

		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = numDescriptors;
		desc.Type = type;

		ThrowIfFailed(m_dx12Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

		return descriptorHeap;
	}

	WRL::ComPtr<ID3D12CommandAllocator> DX12Context::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type)
	{
		WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
		ThrowIfFailed(m_dx12Device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

		return commandAllocator;
	}

	WRL::ComPtr<ID3D12GraphicsCommandList> DX12Context::CreateCommandList(WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type)
	{
		WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
		ThrowIfFailed(m_dx12Device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

		return commandList;
	}

	WRL::ComPtr<ID3D12Fence> DX12Context::CreateFence()
	{
		WRL::ComPtr<ID3D12Fence> fence;

		ThrowIfFailed(m_dx12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

		return fence;
	}
} // namespace gfx