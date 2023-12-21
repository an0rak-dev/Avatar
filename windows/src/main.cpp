
#include <iostream>
#include <vector>
#include "window.h"
#include "dx12_renderer.h"

#define EXIT_OK  0

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#ifdef _DEBUG
#	define APP_NAME L"Avatar [DEBUG]"
#else
#	define APP_NAME L"Avatar"
#endif

#define RENDERER_BUFFERS_COUNT 2

int main() {
	// Init
	HRESULT              result = S_OK;
	Avatar::Core::Window window(WINDOW_WIDTH, WINDOW_HEIGHT, APP_NAME);
	Avatar::Core::Dx12Renderer renderer(RENDERER_BUFFERS_COUNT);

	// Display & Game Loop
	renderer.attachTo(window);
	window.show();

	while (window.pollNextEvent()) {
		renderer.prepareNextFrame();

		renderer.clear(0.0f, 0.0f, 0.0f, 1.0f);

		renderer.present();
	}

	// Shutdown
	return EXIT_OK;
}