#include "Renderer.h"
#include <iostream>
bool GLLogCall(const char* function, const char* file, int line) {
	bool isErrorFree = true;
	while (GLenum error = glGetError()) {
		std::cout << "[OpenGL Error](" << error << ")" << std::endl;
		std::cout << "Function call :: " << function << std::endl;
		std::cout << file << " : " << line << std::endl;
		isErrorFree = false;
	}
	return isErrorFree;
}
void GLClearError() {
	while (glGetError() != GL_NO_ERROR);
}