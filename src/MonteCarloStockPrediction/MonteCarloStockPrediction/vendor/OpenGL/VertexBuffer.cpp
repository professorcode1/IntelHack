#include "VertexBuffer.h"

VertexBuffer::VertexBuffer(const void* data, unsigned int size) {
    std::cout << "Entered" << std::endl;
    glGenBuffers(1, &m_RendererID);
    std::cout << 1 << std::endl;
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
    std::cout << 2 << std::endl;
    GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
    std::cout << "Exiting" << std::endl;
}
VertexBuffer::~VertexBuffer() {
    GLCall(glDeleteBuffers(1, &m_RendererID));
}
void VertexBuffer::Bind()const {
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
}
void VertexBuffer::Unbind()const {
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}