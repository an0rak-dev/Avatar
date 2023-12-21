#include "dx12_renderer.h"
#include "renderer_exception.h"
#include "window.h"
#include <iostream>

using namespace Avatar::Core;

#define SAFE_RELEASE(ptr) \
	if (nullptr != (ptr)) (ptr)->Release();

Dx12Renderer::Dx12Renderer(unsigned int buffersCount) : rtvCount(buffersCount) {
	HRESULT dx12Result = S_OK;
#ifdef _DEBUG
	ID3D12Debug *dx12Debug = nullptr;
	D3D12GetDebugInterface(IID_PPV_ARGS(&dx12Debug));
	if (dx12Result == S_OK) {
		dx12Debug->EnableDebugLayer();
	}
#endif

	dx12Result = CreateDXGIFactory1(IID_PPV_ARGS(&this->factory));
	if (dx12Result != S_OK) {
		throw RendererException("Unable to init the DirectX 12 factory");
	}

	this->findBestAdapter();

	dx12Result = D3D12CreateDevice(this->adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&this->device));
	if (dx12Result != S_OK) {
		throw RendererException("Unable to init the DirectX 12 device");
	}

	this->initCommandMembers();

	const D3D12_DESCRIPTOR_HEAP_DESC
		descriptorHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV, this->rtvCount, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };
	if (S_OK != this->device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&this->descriptorHeap))) {
		throw RendererException("Unable to init the DirectX 12 descriptor heap");
	}
	this->initFenceElements();
}

Dx12Renderer::~Dx12Renderer() {
	for (unsigned int i = 0; i < this->rendertargetviews.size(); i++) {
		SAFE_RELEASE(this->rendertargetviews[i]);
	}
	SAFE_RELEASE(this->swapchain);
	SAFE_RELEASE(this->fence);
	SAFE_RELEASE(this->descriptorHeap);
	SAFE_RELEASE(this->pso);
	SAFE_RELEASE(this->cmdList);
	SAFE_RELEASE(this->cmdAlloc);
	SAFE_RELEASE(this->cmdQueue);
	SAFE_RELEASE(this->device);
	SAFE_RELEASE(this->adapter);
	SAFE_RELEASE(this->factory);
};

void Dx12Renderer::attachTo(const Window &window) {
	const int                   sampleCount   = 1;
	const DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {
		0,
		0, // By default DXGI_SWAP_CHAIN_DESC1 will take the window dimensions if those are 0.
		DXGI_FORMAT_R8G8B8A8_UNORM,
		0,
		{sampleCount, 0},
		DXGI_USAGE_RENDER_TARGET_OUTPUT,
		this->rtvCount,
		DXGI_SCALING_STRETCH,
		DXGI_SWAP_EFFECT_FLIP_DISCARD,
		DXGI_ALPHA_MODE_UNSPECIFIED,
		0
	};
	IDXGISwapChain1 *tmpSwapchain = nullptr;
	HRESULT          dx12Result   = S_OK;
	dx12Result =
		this->factory
			->CreateSwapChainForHwnd(this->cmdQueue, getHandle(window), &swapchainDesc, nullptr, nullptr, &tmpSwapchain);
	if (S_OK != dx12Result) {
		throw RendererException("Unable to init the DirectX 12 swapchain");
	}
	this->swapchain = (IDXGISwapChain3 *) tmpSwapchain;

	D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = this->descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	const unsigned int          descriptorUnitSize =
		this->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (unsigned int i = 0; i < this->rtvCount; i++) {
		ID3D12Resource *currentRtv = nullptr;
		if (S_OK != this->swapchain->GetBuffer(i, IID_PPV_ARGS(&currentRtv))) {
			throw RendererException("Unable to init one of the DirectX 12 render targets");
		}
		this->device->CreateRenderTargetView(currentRtv, nullptr, descriptorHandle);
		this->rendertargetviews.push_back(currentRtv);
		descriptorHandle.ptr += descriptorUnitSize;
	}
}

void Dx12Renderer::prepareNextFrame() {
	if (S_OK != this->cmdAlloc->Reset()) {
		std::cerr << "Unable to reset the DirectX12 renderer (cmdal)" << std::endl;
	}
	if (S_OK != this->cmdList->Reset(this->cmdAlloc, this->pso)) {
		std::cerr << "Unable to reset the DirectX12 renderer (cmdli)" << std::endl;
	}
	this->transitionToPresentMode(false);
}

void Dx12Renderer::present() {
	const int presentSyncInterval = 1;
	this->transitionToPresentMode(true);
	this->cmdList->Close();
	std::vector<ID3D12CommandList *> commandLists = { (ID3D12CommandList *) this->cmdList };
	this->cmdQueue->ExecuteCommandLists((unsigned int) commandLists.size(), commandLists.data());
	if (S_OK != this->swapchain->Present(presentSyncInterval, 0)) {
		std::cerr << "Error while presenting the next DirectX12 renderer frame" << std::endl;
	}
	this->waitForCompletion();
}

void Dx12Renderer::clear(float red, float green, float blue, float alpha) {
	const float                 bgColor[]        = { red, green, blue, alpha };
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle        = this->descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	const unsigned int          currentBufferIdx = this->swapchain->GetCurrentBackBufferIndex();
	const unsigned int          descriptorUnitSize =
		this->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	rtvHandle.ptr += (size_t) (currentBufferIdx * descriptorUnitSize);
	this->cmdList->ClearRenderTargetView(rtvHandle, bgColor, 0, nullptr);
}

void Dx12Renderer::findBestAdapter() {
	HRESULT            dx12Result  = S_OK;
	DXGI_ADAPTER_DESC1 adapterDesc = {};
	const int          maxAdapters = 20; // Huge system if 20 GPU
	for (int i = 0; i < maxAdapters; i++) {
		IDXGIAdapter1 *tmpAdapter = nullptr;
		dx12Result                = this->factory->EnumAdapters1(i, &tmpAdapter);
		if (dx12Result != S_OK) {
			break;
		}
		DXGI_ADAPTER_DESC1 tmpDescription;
		tmpAdapter->GetDesc1(&tmpDescription);

		if (nullptr == this->adapter || (adapterDesc.DedicatedVideoMemory < tmpDescription.DedicatedVideoMemory)) {
			adapterDesc   = tmpDescription;
			this->adapter = tmpAdapter;
		}
	}
	if (nullptr == this->adapter) {
		throw RendererException("Unable to find a suitable DirectX12 adapter");
	}
	std::wcout << L"Using GPU " << adapterDesc.Description << std::endl;
}

void Dx12Renderer::initCommandMembers() {
	HRESULT                        dx12Result   = S_OK;
	const D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
		D3D12_COMMAND_QUEUE_FLAG_NONE,
		0
	};
	if (S_OK != this->device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&this->cmdQueue))) {
		throw RendererException("Unable to init the DirectX 12 command queue");
	}

	if (S_OK != this->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&this->cmdAlloc))) {
		throw RendererException("Unable to init the DirectX 12 command allocator");
	}

	dx12Result = this->device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		this->cmdAlloc,
		this->pso,
		IID_PPV_ARGS(&this->cmdList));
	if (S_OK != dx12Result) {
		throw RendererException("Unable to init the DirectX 12 command list");
	}
	this->cmdList->Close();
}

void Dx12Renderer::initFenceElements() {
	if (S_OK != this->device->CreateFence(this->fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->fence))) {
		throw RendererException("Unable to init the DirectX 12 fence");
	}
	this->fenceEvent = CreateEvent(nullptr, 0, 0, nullptr);
	if (nullptr == this->fenceEvent) {
		throw RendererException("Unable to init the Windows fence event");
	}
	this->waitForCompletion();
}

void Dx12Renderer::waitForCompletion() {
	const unsigned int currentValue = this->fenceValue;
	if (S_OK != this->cmdQueue->Signal(this->fence, currentValue)) {
		std::cerr << "Error while waiting for the next DirectX12 frame" << std::endl;
	}
	if (this->fence->GetCompletedValue() < currentValue) {
		this->fence->SetEventOnCompletion(currentValue, this->fenceEvent);
		WaitForSingleObject(this->fenceEvent, INFINITE);
	}
	this->fenceValue++;
}

void Dx12Renderer::transitionToPresentMode(bool present) {
	const unsigned int     currentBufferIdx = this->swapchain->GetCurrentBackBufferIndex();
	const unsigned int     barrierCount     = 1;
	D3D12_RESOURCE_BARRIER barrier          = {};
	barrier.Type                            = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags                           = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource            = this->rendertargetviews[currentBufferIdx];
	barrier.Transition.StateBefore = (present) ? D3D12_RESOURCE_STATE_RENDER_TARGET : D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter  = (present) ? D3D12_RESOURCE_STATE_PRESENT : D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	this->cmdList->ResourceBarrier(barrierCount, &barrier);
}