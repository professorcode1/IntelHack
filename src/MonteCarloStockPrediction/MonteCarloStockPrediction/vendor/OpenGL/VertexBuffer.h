#pragma once 
#include "Renderer.h"
#include <iostream>
class VertexBuffer {
private:
    unsigned int m_RendererID;
public:
    VertexBuffer(const void* data, unsigned int size);
    ~VertexBuffer();
    void Bind()const;
    void Unbind()const;
};