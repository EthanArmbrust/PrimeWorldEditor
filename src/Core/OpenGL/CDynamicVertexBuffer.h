#ifndef CDYNAMICVERTEXBUFFER_H
#define CDYNAMICVERTEXBUFFER_H

#include <GL/glew.h>
#include <Common/types.h>
#include <Common/CVector2f.h>
#include <Common/CVector3f.h>
#include <Resource/model/EVertexDescription.h>
#include <vector>

class CDynamicVertexBuffer
{
    EVertexDescription mAttribFlags;
    EVertexDescription mBufferedFlags;
    u32 mNumVertices;
    GLuint mAttribBuffers[12];

public:
    CDynamicVertexBuffer();
    ~CDynamicVertexBuffer();
    void SetVertexCount(u32 NumVerts);
    void Bind();
    void Unbind();
    void SetActiveAttribs(u32 AttribFlags);
    void BufferAttrib(EVertexDescription Attrib, const void *pData);
    void ClearBuffers();
    GLuint CreateVAO();
private:
    void InitBuffers();
};

#endif // CDYNAMICVERTEXBUFFER_H