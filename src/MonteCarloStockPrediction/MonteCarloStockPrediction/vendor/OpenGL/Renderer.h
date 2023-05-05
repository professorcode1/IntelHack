#pragma once
#define ASSERT(x) if(!(x)) abort();
/*#define GLCall(x) GLClearError();\
	x;\
	ASSERT(GLLogCall(#x, __FILE__, __LINE__))
*/

#define GLCall(x) x
#include <GL/glew.h>
#include <stdlib.h>

bool GLLogCall(const char* function, const char* file, int line);
void GLClearError();