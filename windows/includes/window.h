#ifndef WINDOW_H
#define WINDOW_H

#include <Windows.h>

namespace Avatar::Core {
	/**
	 * @brief  Window represent a Graphic User Interface where the app output will be displayed.
	 */
	class Window {
		public:
			/**
			 * @brief Create a new window object attached to the current running process.
			 *
			 * @param width the wanted width of the window
			 * @param height the wanted height of the window
			 * @param app_name the wand window's name as it should be displayed in the title bar.
			 */
			Window(int width, int height, const wchar_t *app_name);
			virtual ~Window();

			/**
			 * @brief Show the window on screen.
			 */
			void show();

			/**
			 * @brief Poll the next input event received by the window. This call is a blocking one.
			 *
			 * @return true if an event was polled, false if the event received is a quit event.
			 */
			bool pollNextEvent();

			friend HWND getHandle(const Window &window);

		private:
			HWND windowHandle = nullptr;
	};
}

#endif