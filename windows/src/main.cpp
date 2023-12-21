
#include "dx12_renderer.h"
#include "window.h"

#define EXIT_OK 0

#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720
#ifdef _DEBUG
#	define APP_NAME L"Avatar [DEBUG]"
#else
#	define APP_NAME L"Avatar"
#endif

#define RENDERER_BUFFERS_COUNT 2

int main() {
	// Init
	Avatar::Core::Window       window(WINDOW_WIDTH, WINDOW_HEIGHT, APP_NAME);
	Avatar::Core::Dx12Renderer renderer(RENDERER_BUFFERS_COUNT);

	// Display & Game Loop
	renderer.attachTo(window);
	window.show();

	while (window.pollNextEvent()) {
		renderer.prepareNextFrame();

		renderer.clear(0.0F, 0.0F, 0.0F, 1.0F);

		renderer.present();
	}

	// Shutdown
	return EXIT_OK;
}