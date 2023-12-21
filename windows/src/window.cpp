#include "window.h"
#include <iostream>

using namespace Avatar::Core;

LRESULT CALLBACK windowProcedure(HWND window_handle, UINT message, WPARAM wide_params, LPARAM long_params) {
	if (WM_CLOSE == message) {
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(window_handle, message, wide_params, long_params);
}

Window::Window(int width, int height, const wchar_t *app_name) {
	HINSTANCE        currentProcess = GetModuleHandle(nullptr);
	const WNDCLASSEX windowClass    = {
      sizeof(WNDCLASSEX),
      CS_HREDRAW | CS_VREDRAW,
      windowProcedure,
      0,
      0,
      currentProcess,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      L"AvatarWindowClass",
      nullptr
	};
	if (0 == RegisterClassEx(&windowClass)) {
		std::cerr << "Unable to register the window class" << std::endl;
		this->windowHandle = nullptr;
		return;
	}

	this->windowHandle = CreateWindowEx(
		WS_EX_OVERLAPPEDWINDOW,
		L"AvatarWindowClass",
		app_name,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		nullptr,
		nullptr,
		currentProcess,
		nullptr);

	if (nullptr == this->windowHandle) {
		std::cerr << "Unable to create the window handle" << std::endl;
		return;
	}
}

Window::~Window() {
	if (nullptr != this->windowHandle) {
		DestroyWindow(this->windowHandle);
	}
}

void Window::show() {
	if (nullptr != this->windowHandle) {
		ShowWindow(this->windowHandle, SW_SHOW);
	}
}

bool Window::pollNextEvent() {
	MSG msg;
	if (nullptr != this->windowHandle && GetMessage(&msg, this->windowHandle, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		return true;
	}
	return false;
}

HWND Avatar::Core::getHandle(const Window &window) {
	return window.windowHandle;
}