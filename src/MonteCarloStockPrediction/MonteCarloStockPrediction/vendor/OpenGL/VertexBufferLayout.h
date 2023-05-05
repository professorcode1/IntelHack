#pragma once
#include "Renderer.h"
#include <vector>
struct VertexBufferElement {
    unsigned int type;
    unsigned int count;
    unsigned char normalised;

    static unsigned int GetSizeOfType(unsigned int type) {
        switch (type) {
        case GL_FLOAT:         return sizeof(GLfloat);
        case GL_UNSIGNED_INT:  return sizeof(GLuint);
        case GL_UNSIGNED_BYTE: return sizeof(GLbyte);
        }
        ASSERT(false);
        return 0;
    }
};
class VertexBufferLayout {
private:
    std::vector<VertexBufferElement> m_Elements;
    unsigned int m_Stride;
public:
    VertexBufferLayout() : m_Stride{ 0 } {}
    template<typename T>
    void Push(unsigned int count) {
        static_assert(true);
    }

    inline const std::vector<VertexBufferElement>& GetElements() const { return m_Elements; };
    inline unsigned int GetStride() const { return m_Stride; }
};