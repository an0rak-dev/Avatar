#ifndef DX12_RENDERER_H
#define DX12_RENDERER_H

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h> // TODO: Get rid of ComPtr by something less Windows specific
#include <vector>
#include "window.h"

namespace Avatar {
	namespace Core {
		class Dx12Renderer {
			public:
				Dx12Renderer(unsigned int buffers_count);
				virtual ~Dx12Renderer();

				void attachTo(const Window& window);
				void prepareNextFrame();
				void present();

				void clear(float red, float green, float blue, float alpha);

			private:
				void waitForCompletion();
				void transitionToPresentMode(bool present);

				const unsigned int rtv_count;
				unsigned int                                      fence_value = 0;
				Microsoft::WRL::ComPtr<IDXGIFactory2> factory;
				Microsoft::WRL::ComPtr<IDXGIAdapter>  adapter;
				Microsoft::WRL::ComPtr<ID3D12Device>  device;
				Microsoft::WRL::ComPtr<ID3D12CommandQueue> cmd_queue;
				Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmd_alloc;
				Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmd_list;
				Microsoft::WRL::ComPtr<ID3D12PipelineState>       pso;
				Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptor_heap;
				Microsoft::WRL::ComPtr<ID3D12Fence>               fence;
				HANDLE                                            fence_event = NULL;
				Microsoft::WRL::ComPtr<IDXGISwapChain3>	swapchain;
				std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> rendertargetviews;
		};
	};
};

#endif