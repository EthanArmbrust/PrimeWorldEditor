#ifndef CGRIDRENDERABLE
#define CGRIDRENDERABLE

#include <Core/Render/CRenderer.h>
#include <Core/Render/IRenderable.h>

// Tiny helper to make sure the grid draws at the correct depth.
class CGridRenderable : public IRenderable
{
    CColor mLineColor;
    CColor mBoldLineColor;

public:
    CGridRenderable()
        : mLineColor(0.6f, 0.6f, 0.6f, 0.f)
        , mBoldLineColor(0.f, 0.f, 0.f, 0.f)
    {}

    inline void SetColor(const CColor& rkLineColor, const CColor& rkBoldColor)
    {
        mLineColor = rkLineColor;
        mBoldLineColor = rkBoldColor;
    }

    void AddToRenderer(CRenderer *pRenderer, const SViewInfo&)
    {
        pRenderer->AddMesh(this, 0, CAABox::skOne, false, ERenderCommand::DrawMesh);
    }

    void Draw(FRenderOptions, int, ERenderCommand, const SViewInfo&)
    {
        CDrawUtil::DrawGrid(mLineColor, mBoldLineColor);
    }
};

#endif // CGRIDRENDERABLE

