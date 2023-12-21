#ifndef DX12_RENDERER_H
#define DX12_RENDERER_H

#include "window.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <vector>

namespace Avatar::Core {
	/**
	 * @brief Define operations to perform graphic computing on the GPU using DirectX 12.
	 */
	class Dx12Renderer {
		public:
			/**
			 * @brief Creates a new renderer for the given numbers of buffers (including the front buffer).
			 *
			 * @param buffersCount the total number of buffers to use (front + back buffers).
			 * @throws RendererException if one of the call to the DirectX12 api fails.
			 */
			Dx12Renderer(unsigned int buffersCount);
			virtual ~Dx12Renderer();

			/**
			 * @brief Set up the given window has the renderering target for this renderer.
			 *
			 * Create the swapchain and render target buffers dedicated to the given Window.
			 *
			 * @param window the window to use has a target for the rendering operations.
			 * @throws RendererException if one of the call to the DirectX12 api fails.
			 */
			void attachTo(const Window &window);

			/**
			 * @brief Reset all the command lists & allocators and transition the renderer to a recording state.
			 */
			void prepareNextFrame();

			/**
			 * @brief Submit the command lists to the GPU and transition the renderer to a presenting state.
			 */
			void present();

			/**
			 * @brief Append a clear command to the command lists to clear all the rendering surface with a solid color.
			 *
			 * @param red the red component of the color (0.0 : none, 1.0 : full)
			 * @param green the green component of the color (0.0 : none, 1.0 : full)
			 * @param blue the blue component of the color (0.0 : none, 1.0 : full)
			 * @param alpha the opacity component of the color (0.0 : transparent, 1.0 : opaque)
			 */
			void clear(float red, float green, float blue, float alpha);

		private:
			void findBestAdapter();
			void initCommandMembers();
			void initFenceElements();
			void waitForCompletion();
			void transitionToPresentMode(bool present);

			unsigned int                  rtvCount;
			unsigned int                  fenceValue     = 0;
			IDXGIFactory2                *factory        = nullptr;
			IDXGIAdapter                 *adapter        = nullptr;
			ID3D12Device                 *device         = nullptr;
			ID3D12CommandQueue           *cmdQueue       = nullptr;
			ID3D12CommandAllocator       *cmdAlloc       = nullptr;
			ID3D12GraphicsCommandList    *cmdList        = nullptr;
			ID3D12PipelineState          *pso            = nullptr;
			ID3D12DescriptorHeap         *descriptorHeap = nullptr;
			ID3D12Fence                  *fence          = nullptr;
			HANDLE                        fenceEvent     = nullptr;
			IDXGISwapChain3              *swapchain      = nullptr;
			std::vector<ID3D12Resource *> rendertargetviews;
	};
};

#endif