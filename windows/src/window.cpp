#include "window.h"
#include <iostream>

using namespace Avatar::Core;

LRESULT CALLBACK window_procedure(HWND window_handle, UINT message, WPARAM wide_params, LPARAM long_params) {
	if (WM_CLOSE == message) {
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(window_handle, message, wide_params, long_params);
}

Window::Window(unsigned int width, unsigned int height, const wchar_t *app_name) {
	HINSTANCE  current_process = GetModuleHandle(NULL);
	WNDCLASSEX window_class    = {
      sizeof(WNDCLASSEX),
      CS_HREDRAW | CS_VREDRAW,
      window_procedure,
      0,
      0,
      current_process,
      NULL,
      NULL,
      NULL,
      NULL,
      L"AvatarWindowClass",
      NULL
	};
	if (0 == RegisterClassEx(&window_class)) {
		std::cerr << "Unable to register the window class" << std::endl;
		this->window_handle = NULL;
		return;
	}

	this->window_handle = CreateWindowEx(
		WS_EX_OVERLAPPEDWINDOW,
		L"AvatarWindowClass",
		app_name,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		NULL,
		NULL,
		current_process,
		NULL);

	if (NULL == this->window_handle) {
		std::cerr << "Unable to create the window handle" << std::endl;
		return;
	}
}

Window::~Window() {
	if (NULL != this->window_handle) {
		DestroyWindow(this->window_handle);
	}
}

void Window::show() {
	if (NULL != this->window_handle) {
		ShowWindow(this->window_handle, SW_SHOW);
	}
}

bool Window::pollNextEvent() {
	MSG msg;
	if (NULL != this->window_handle && GetMessage(&msg, this->window_handle, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		return true;
	}
	return false;
}

HWND Window::getHandle() const {
	return this->window_handle;
}