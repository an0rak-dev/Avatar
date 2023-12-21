#ifndef RENDERER_EXCEPTION_H
#define RENDERER_EXCEPTION_H

#include <exception>

namespace Avatar::Core {
	class RendererException : public std::exception {
		public:
			RendererException(const char *cause) : msg(cause) {}

			~RendererException() override = default;

			const char *what() const throw() override { return this->msg; }

		private:
			const char *msg;
	};
}

#endif