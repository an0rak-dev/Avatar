#include "dx12_renderer.h"

#include <iostream>


using namespace Avatar::Core;

Dx12Renderer::Dx12Renderer(unsigned int buffers_count) : rtv_count(buffers_count) {
	// TODO: Find a way to handle properly errors _without_ exceptions.
	HRESULT dx12_result = S_OK;
#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug> dx12_debug;
	dx12_result = D3D12GetDebugInterface(IID_PPV_ARGS(&dx12_debug));
	if (dx12_result != S_OK) {
		std::cerr << "Unable to enable the DirectX 12 debug layer" << std::endl;
	} else {
		dx12_debug->EnableDebugLayer();
	}
#endif

	dx12_result = CreateDXGIFactory1(IID_PPV_ARGS(&this->factory));
	if (dx12_result != S_OK) {
		std::cerr << "Unable to init the DirectX12 renderer (factory)" << std::endl;
	}
	
	DXGI_ADAPTER_DESC1 adapter_desc = {};
	const int          max_adapters = 20; // Huge system if 20 GPU
	for (int i = 0; i < max_adapters; i++) {
		IDXGIAdapter1 *tmp_adapter;
		dx12_result = this->factory->EnumAdapters1(i, &tmp_adapter);
		if (dx12_result != S_OK) {
			break;
		}
		DXGI_ADAPTER_DESC1 tmp_description;
		tmp_adapter->GetDesc1(&tmp_description);

		if (NULL == this->adapter || (adapter_desc.DedicatedVideoMemory < tmp_description.DedicatedVideoMemory)) {
			adapter_desc = tmp_description;
			this->adapter = tmp_adapter;
		}
	}
	if (NULL == this->adapter) {
		std::cerr << "Unable to init the DirectX12 renderer (adapter)" << std::endl;
	}
	std::wcout << L"Using GPU " << adapter_desc.Description << std::endl;

	dx12_result = D3D12CreateDevice(this->adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&this->device));
	if (dx12_result != S_OK) {
		std::cerr << "Unable to init the DirectX12 renderer (device)" << std::endl;
	}

	D3D12_COMMAND_QUEUE_DESC cmd_queue_desc;
	cmd_queue_desc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
	cmd_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmd_queue_desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmd_queue_desc.NodeMask = 0;
	if (S_OK != this->device->CreateCommandQueue(&cmd_queue_desc, IID_PPV_ARGS(&this->cmd_queue))) {
		std::cerr << "Unable to init the DirectX12 renderer (cmdqu)" << std::endl;
	}

	if (S_OK != this->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&this->cmd_alloc))) {
		std::cerr << "Unable to init the DirectX12 renderer (cmdal)" << std::endl;
	}

	dx12_result = this->device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		this->cmd_alloc.Get(),
		this->pso.Get(),
		IID_PPV_ARGS(&this->cmd_list));
	if (S_OK != dx12_result) {
		std::cerr << "Unable to init the DirectX12 renderer (cmdli)" << std::endl;
	}
	this->cmd_list->Close();

	D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc;
	descriptor_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descriptor_heap_desc.NumDescriptors = this->rtv_count;
	descriptor_heap_desc.NodeMask       = 0;
	descriptor_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (S_OK != this->device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&this->descriptor_heap))) {
		std::cerr << "Unable to init the DirectX12 renderer (descheap)" << std::endl;
	}

	if (S_OK != this->device->CreateFence(this->fence_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->fence))) {
		std::cerr << "Unable to init the DirectX12 renderer (fence)" << std::endl;
	}
	this->fence_event = CreateEvent(NULL, 0, 0, NULL);
	if (NULL == this->fence_event) {
		std::cerr << "Unable to init the DirectX12 renderer (fenceev)" << std::endl;
	}
	this->waitForCompletion();
}

Dx12Renderer::~Dx12Renderer() {

}

void Dx12Renderer::attachTo(const Window &window) {
	DXGI_SWAP_CHAIN_DESC1 swapchain_desc;
	const int             sample_count = 1;
	swapchain_desc.Width               = 0;
	swapchain_desc.Height = 0; // By default DXGI_SWAP_CHAIN_DESC1 will take the window dimensions if those are 0.
	swapchain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchain_desc.Stereo = 0;
	swapchain_desc.SampleDesc.Count   = sample_count;
	swapchain_desc.SampleDesc.Quality = 0;
	swapchain_desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchain_desc.BufferCount        = this->rtv_count;
	swapchain_desc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchain_desc.Scaling            = DXGI_SCALING_STRETCH;
	swapchain_desc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchain_desc.Flags              = 0;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> tmp_swapchain;
	// TODO Expose window properly.
	HRESULT dx12_result = this->factory->CreateSwapChainForHwnd(
		this->cmd_queue.Get(),
		window.getHandle(),
		&swapchain_desc,
		NULL,
		NULL,
		&tmp_swapchain);
	if (S_OK != dx12_result) {
		std::cerr << "Unable to init the DirectX12 renderer (swapc)" << std::endl;
	} else {
		tmp_swapchain.As(&this->swapchain);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE descriptor_handle = this->descriptor_heap->GetCPUDescriptorHandleForHeapStart();
	const unsigned int          descriptor_unit_size =
		this->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (int i = 0; i < this->rtv_count; i++) {
		Microsoft::WRL::ComPtr<ID3D12Resource> current_rtv;
		if (S_OK != this->swapchain->GetBuffer(i, IID_PPV_ARGS(&current_rtv))) {
			std::cerr << "Unable to init the DirectX12 renderer (rtv " << i << ")" << std::endl;
		}
		this->device->CreateRenderTargetView(current_rtv.Get(), NULL, descriptor_handle);
		this->rendertargetviews.push_back(current_rtv);
		descriptor_handle.ptr += descriptor_unit_size;
	}
}

void Dx12Renderer::prepareNextFrame() {
	if (S_OK != this->cmd_alloc->Reset()) {
		std::cerr << "Unable to reset the DirectX12 renderer (cmdal)" << std::endl;
	}
	if (S_OK != this->cmd_list->Reset(this->cmd_alloc.Get(), this->pso.Get())) {
		std::cerr << "Unable to reset the DirectX12 renderer (cmdli)" << std::endl;
	}
	this->transitionToPresentMode(false);
}

void Dx12Renderer::present() {
	const int present_sync_interval = 1;
	this->transitionToPresentMode(true);
	this->cmd_list->Close();
	std::vector<ID3D12CommandList *> command_lists = { (ID3D12CommandList *) this->cmd_list.Get() };
	this->cmd_queue->ExecuteCommandLists(command_lists.size(), command_lists.data());
	if (S_OK != this->swapchain->Present(present_sync_interval, 0)) {
		std::cerr << "Error while presenting the next DirectX12 renderer frame" << std::endl;
	}
	this->waitForCompletion();
}

void Dx12Renderer::clear(float red, float green, float blue, float alpha) {
	const float                 bg_color[] = { red, green, blue, alpha };
	D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = this->descriptor_heap->GetCPUDescriptorHandleForHeapStart();
	const unsigned int          current_buffer_idx = this->swapchain->GetCurrentBackBufferIndex();
	const unsigned int          descriptor_unit_size =
		this->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	rtv_handle.ptr += current_buffer_idx * descriptor_unit_size;
	this->cmd_list->ClearRenderTargetView(rtv_handle, bg_color, 0, NULL);
}

void Dx12Renderer::waitForCompletion() {
	const unsigned int current_value = this->fence_value;
	if (S_OK != this->cmd_queue->Signal(this->fence.Get(), current_value)) {
		std::cerr << "Error while waiting for the next DirectX12 frame" << std::endl;
	}
	if (this->fence->GetCompletedValue() < current_value) {
		this->fence->SetEventOnCompletion(current_value, this->fence_event);
		WaitForSingleObject(this->fence_event, INFINITE);
	}
	this->fence_value++;
}

void Dx12Renderer::transitionToPresentMode(bool present) {
	const unsigned int     current_buffer_idx = this->swapchain->GetCurrentBackBufferIndex();
	const unsigned int     barrier_count = 1;
	D3D12_RESOURCE_BARRIER barrier       = {};
	barrier.Type                         = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags                        = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource         = this->rendertargetviews[current_buffer_idx].Get();
	barrier.Transition.StateBefore       = (present) ? D3D12_RESOURCE_STATE_RENDER_TARGET : D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter        = (present) ? D3D12_RESOURCE_STATE_PRESENT : D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource       = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	this->cmd_list->ResourceBarrier(barrier_count, &barrier);
}