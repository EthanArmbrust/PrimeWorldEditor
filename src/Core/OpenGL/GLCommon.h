#ifndef GLCOMMON_H
#define GLCOMMON_H

#include <GL/glew.h>
#include <Common/types.h>

enum EBlendFactor
{
    eBlendZero        = GL_ZERO,
    eBlendOne         = GL_ONE,
    eBlendSrcColor    = GL_SRC_COLOR,
    eBlendInvSrcColor = GL_ONE_MINUS_SRC_COLOR,
    eBlendSrcAlpha    = GL_SRC_ALPHA,
    eBlendInvSrcAlpha = GL_ONE_MINUS_SRC_ALPHA,
    eBlendDstAlpha    = GL_DST_ALPHA,
    eBlendInvDstAlpha = GL_ONE_MINUS_DST_ALPHA
};

enum EGXPrimitiveType
{
    eGX_Quads = 0x80,
    eGX_Triangles = 0x90,
    eGX_TriangleStrip = 0x98,
    eGX_TriangleFan = 0xA0,
    eGX_Lines = 0xA8,
    eGX_LineStrip = 0xB0,
    eGX_Points = 0xB8
};

extern GLenum glBlendFactor[];
extern GLenum glZMode[];
GLenum GXPrimToGLPrim(EGXPrimitiveType t);

#endif // GLCOMMON_H