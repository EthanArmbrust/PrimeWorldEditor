#include "CScriptNode.h"
#include "CScene.h"
#include "Core/Render/CDrawUtil.h"
#include "Core/Render/CGraphics.h"
#include "Core/Render/CRenderer.h"
#include "Core/Resource/CResCache.h"
#include "Core/Resource/Script/CMasterTemplate.h"
#include "Core/Resource/Script/CScriptLayer.h"
#include "Core/ScriptExtra/CScriptExtra.h"
#include <Math/MathUtil.h>

CScriptNode::CScriptNode(CScene *pScene, u32 NodeID, CSceneNode *pParent, CScriptObject *pInstance)
    : CSceneNode(pScene, NodeID, pParent)
    , mGameModeVisibility(eUntested)
    , mpVolumePreviewNode(nullptr)
    , mHasVolumePreview(false)
    , mpInstance(pInstance)
    , mpBillboard(nullptr)
{

    // Evaluate instance
    SetActiveModel(nullptr);
    mpCollisionNode = new CCollisionNode(pScene, -1, this);
    mpCollisionNode->SetInheritance(true, true, false);

    if (mpInstance)
    {
        CScriptTemplate *pTemp = Template();

        // Determine transform
        mHasValidPosition = pTemp->HasPosition();
        mPosition = mpInstance->Position();
        mRotation = CQuaternion::FromEuler(mpInstance->Rotation());
        mScale = mpInstance->Scale();
        MarkTransformChanged();

        SetName("[" + pTemp->Name() + "] " + mpInstance->InstanceName());

        // Determine display assets
        SetActiveModel(mpInstance->GetDisplayModel());
        mpBillboard = mpInstance->GetBillboard();
        mpCollisionNode->SetCollision(mpInstance->GetCollision());

        // Create preview volume node
        mpVolumePreviewNode = new CModelNode(pScene, -1, this, nullptr);

        if (pTemp->ScaleType() == CScriptTemplate::eScaleVolume)
        {
            UpdatePreviewVolume();

            if (mHasVolumePreview)
            {
                mpVolumePreviewNode->SetInheritance(true, (mpInstance->VolumeShape() != eAxisAlignedBoxShape), true);
                mpVolumePreviewNode->ForceAlphaEnabled(true);
            }
        }

        // Fetch LightParameters
        mpLightParameters = new CLightParameters(mpInstance->LightParameters(), mpInstance->MasterTemplate()->Game());
        SetLightLayerIndex(mpLightParameters->LightLayerIndex());
    }

    else
    {
        // Shouldn't ever happen
        SetName("ScriptNode - NO INSTANCE");
    }

    mpExtra = CScriptExtra::CreateExtra(this);
}

ENodeType CScriptNode::NodeType()
{
    return eScriptNode;
}

void CScriptNode::PostLoad()
{
    if (mpActiveModel)
    {
        mpActiveModel->BufferGL();
        mpActiveModel->GenerateMaterialShaders();
    }
}

void CScriptNode::OnTransformed()
{
    if (mpInstance)
    {
        CScriptTemplate *pTemplate = Template();

        if (pTemplate->HasPosition() && LocalPosition() != mpInstance->Position())
            mpInstance->SetPosition(LocalPosition());

        if (pTemplate->HasRotation() && LocalRotation().ToEuler() != mpInstance->Rotation())
            mpInstance->SetRotation(LocalRotation().ToEuler());

        if (pTemplate->HasScale() && LocalScale() != mpInstance->Scale())
            mpInstance->SetScale(LocalScale());
    }

    if (mpExtra)
        mpExtra->OnTransformed();
}

void CScriptNode::AddToRenderer(CRenderer *pRenderer, const SViewInfo& rkViewInfo)
{
    if (!mpInstance) return;

    // Add script extra to renderer first
    if (mpExtra) mpExtra->AddToRenderer(pRenderer, rkViewInfo);

    // If we're in game mode, then override other visibility settings.
    if (rkViewInfo.GameMode)
    {
        if (mGameModeVisibility == eUntested)
            TestGameModeVisibility();

        if (mGameModeVisibility != eVisible)
            return;
    }

    // Check whether the script extra wants us to render before we render.
    bool ShouldDraw = (!mpExtra || mpExtra->ShouldDrawNormalAssets());

    if (ShouldDraw)
    {
        // Otherwise, we proceed as normal
        if ((rkViewInfo.ShowFlags & eShowObjectCollision) && (!rkViewInfo.GameMode))
            mpCollisionNode->AddToRenderer(pRenderer, rkViewInfo);

        if (rkViewInfo.ShowFlags & eShowObjectGeometry || rkViewInfo.GameMode)
        {
            if (rkViewInfo.ViewFrustum.BoxInFrustum(AABox()))
            {
                if (!mpActiveModel)
                    pRenderer->AddOpaqueMesh(this, -1, AABox(), eDrawMesh);

                else
                {
                    if (!mpActiveModel->HasTransparency(0))
                        pRenderer->AddOpaqueMesh(this, -1, AABox(), eDrawMesh);
                    else
                        AddSurfacesToRenderer(pRenderer, mpActiveModel, 0, rkViewInfo);
                }
            }
        }
    }

    if (IsSelected() && !rkViewInfo.GameMode)
    {
        // Script nodes always draw their selections regardless of frustum planes
        // in order to ensure that script connection lines don't get improperly culled.
       if (ShouldDraw)
           pRenderer->AddOpaqueMesh(this, -1, AABox(), eDrawSelection);

        if (mHasVolumePreview && (!mpExtra || mpExtra->ShouldDrawVolume()))
            mpVolumePreviewNode->AddToRenderer(pRenderer, rkViewInfo);
    }
}

void CScriptNode::Draw(FRenderOptions Options, int ComponentIndex, const SViewInfo& rkViewInfo)
{
    if (!mpInstance) return;

    // Draw model
    if (UsesModel())
    {
        EWorldLightingOptions LightingOptions = (mpLightParameters ? mpLightParameters->WorldLightingOptions() : eNormalLighting);

        if (CGraphics::sLightMode == CGraphics::eWorldLighting && LightingOptions == eDisableWorldLighting)
        {
            CGraphics::sNumLights = 0;
            CGraphics::sVertexBlock.COLOR0_Amb = CColor::skBlack;
            CGraphics::sPixelBlock.LightmapMultiplier = 1.f;
            CGraphics::UpdateLightBlock();
        }

        else
        {
            // DKCR doesn't support world lighting yet, so light nodes that don't have ingame models with default lighting
            if (Template()->Game() == eReturns && !mpInstance->HasInGameModel() && CGraphics::sLightMode == CGraphics::eWorldLighting)
            {
                CGraphics::SetDefaultLighting();
                CGraphics::sVertexBlock.COLOR0_Amb = CGraphics::skDefaultAmbientColor;
            }

            else
                LoadLights(rkViewInfo);
        }

        LoadModelMatrix();

        // Draw model if possible!
        if (mpActiveModel)
        {
            if (mpExtra) CGraphics::sPixelBlock.TevColor = mpExtra->TevColor();
            else CGraphics::sPixelBlock.TevColor = CColor::skWhite;

            CGraphics::sPixelBlock.TintColor = TintColor(rkViewInfo);
            CGraphics::UpdatePixelBlock();

            if (ComponentIndex < 0)
                mpActiveModel->Draw(Options, 0);
            else
                mpActiveModel->DrawSurface(Options, ComponentIndex, 0);
        }

        // If no model or billboard, default to drawing a purple box
        else
        {
            glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ZERO, GL_ZERO);
            glDepthMask(GL_TRUE);
            CGraphics::UpdateVertexBlock();
            CGraphics::UpdatePixelBlock();
            CDrawUtil::DrawShadedCube(CColor::skTransparentPurple * TintColor(rkViewInfo));
        }
    }

    // Draw billboard
    else if (mpBillboard)
    {
        CDrawUtil::DrawBillboard(mpBillboard, mPosition, BillboardScale(), TintColor(rkViewInfo));
    }
}

void CScriptNode::DrawSelection()
{
    glBlendFunc(GL_ONE, GL_ZERO);
    LoadModelMatrix();

    // Draw wireframe for models
    if (UsesModel())
    {
        CModel *pModel = (mpActiveModel ? mpActiveModel : CDrawUtil::GetCubeModel());
        pModel->DrawWireframe(eNoRenderOptions, WireframeColor());
    }

    // Draw rotation arrow for billboards
    else
    {
        // Create model matrix that doesn't take scaling into account, and draw
        CTransform4f Transform;
        Transform.Rotate(AbsoluteRotation());
        Transform.Translate(AbsolutePosition());
        CGraphics::sMVPBlock.ModelMatrix = Transform.ToMatrix4f();
        CGraphics::UpdateMVPBlock();

        CGraphics::sPixelBlock.TintColor = CColor::skWhite;
        CGraphics::UpdatePixelBlock();

        DrawRotationArrow();
    }

    if (mpInstance)
    {
        CGraphics::sMVPBlock.ModelMatrix = CMatrix4f::skIdentity;
        CGraphics::UpdateMVPBlock();

        for (u32 iIn = 0; iIn < mpInstance->NumLinks(eIncoming); iIn++)
        {
            // Don't draw in links if the other object is selected.
            CLink *pLink = mpInstance->Link(eIncoming, iIn);
            CScriptNode *pLinkNode = mpScene->NodeForInstanceID(pLink->SenderID());
            if (pLinkNode && !pLinkNode->IsSelected()) CDrawUtil::DrawLine(CenterPoint(), pLinkNode->CenterPoint(), CColor::skTransparentRed);
        }

        for (u32 iOut = 0; iOut < mpInstance->NumLinks(eOutgoing); iOut++)
        {
            CLink *pLink = mpInstance->Link(eOutgoing, iOut);
            CScriptNode *pLinkNode = mpScene->NodeForInstanceID(pLink->ReceiverID());
            if (pLinkNode) CDrawUtil::DrawLine(CenterPoint(), pLinkNode->CenterPoint(), CColor::skTransparentGreen);
        }
    }
}

void CScriptNode::RayAABoxIntersectTest(CRayCollisionTester& rTester, const SViewInfo& rkViewInfo)
{
    if (!mpInstance)
        return;

    // Let script extra do ray check first
    if (mpExtra)
    {
        mpExtra->RayAABoxIntersectTest(rTester, rkViewInfo);

        // If the extra doesn't want us rendering, then don't do the ray test either
        if (!mpExtra->ShouldDrawNormalAssets())
            return;
    }

    // If we're in game mode, then check whether we're visible before proceeding with the ray test.
    if (rkViewInfo.GameMode)
    {
        if (mGameModeVisibility == eUntested)
            TestGameModeVisibility();

        if (mGameModeVisibility != eVisible)
            return;
    }

    // Otherwise, proceed with the ray test as normal...
    const CRay& rkRay = rTester.Ray();

    if (UsesModel())
    {
        std::pair<bool,float> BoxResult = AABox().IntersectsRay(rkRay);

        if (BoxResult.first)
        {
            if (mpActiveModel) rTester.AddNodeModel(this, mpActiveModel);
            else rTester.AddNode(this, 0, BoxResult.second);
        }
    }

    else
    {
        // Because the billboard rotates a lot, expand the AABox on the X/Y axes to cover any possible orientation
        CVector2f BillScale = BillboardScale();
        float ScaleXY = (BillScale.X > BillScale.Y ? BillScale.X : BillScale.Y);

        CAABox BillBox = CAABox(mPosition + CVector3f(-ScaleXY, -ScaleXY, -BillScale.Y),
                                mPosition + CVector3f( ScaleXY,  ScaleXY,  BillScale.Y));

        std::pair<bool,float> BoxResult = BillBox.IntersectsRay(rkRay);
        if (BoxResult.first) rTester.AddNode(this, 0, BoxResult.second);
    }
}

SRayIntersection CScriptNode::RayNodeIntersectTest(const CRay& rkRay, u32 AssetID, const SViewInfo& rkViewInfo)
{
    FRenderOptions Options = rkViewInfo.pRenderer->RenderOptions();

    SRayIntersection Out;
    Out.pNode = this;
    Out.ComponentIndex = AssetID;

    // Model test
    if (UsesModel())
    {
        CModel *pModel = (mpActiveModel ? mpActiveModel : CDrawUtil::GetCubeModel());

        CRay TransformedRay = rkRay.Transformed(Transform().Inverse());
        std::pair<bool,float> Result = pModel->GetSurface(AssetID)->IntersectsRay(TransformedRay, ((Options & eEnableBackfaceCull) == 0));

        if (Result.first)
        {
            Out.Hit = true;

            CVector3f HitPoint = TransformedRay.PointOnRay(Result.second);
            CVector3f WorldHitPoint = Transform() * HitPoint;
            Out.Distance = Math::Distance(rkRay.Origin(), WorldHitPoint);
        }

        else
            Out.Hit = false;
    }

    // Billboard test
    // todo: come up with a better way to share this code between CScriptNode and CLightNode
    else
    {
        // Step 1: check whether the ray intersects with the plane the billboard is on
        CPlane BillboardPlane(-rkViewInfo.pCamera->Direction(), mPosition);
        std::pair<bool,float> PlaneTest = Math::RayPlaneIntersecton(rkRay, BillboardPlane);

        if (PlaneTest.first)
        {
            // Step 2: transform the hit point into the plane's local space
            CVector3f PlaneHitPoint = rkRay.PointOnRay(PlaneTest.second);
            CVector3f RelHitPoint = PlaneHitPoint - mPosition;

            CVector3f PlaneForward = -rkViewInfo.pCamera->Direction();
            CVector3f PlaneRight = -rkViewInfo.pCamera->RightVector();
            CVector3f PlaneUp = rkViewInfo.pCamera->UpVector();
            CQuaternion PlaneRot = CQuaternion::FromAxes(PlaneRight, PlaneForward, PlaneUp);

            CVector3f RotatedHitPoint = PlaneRot.Inverse() * RelHitPoint;
            CVector2f LocalHitPoint = RotatedHitPoint.XZ() / BillboardScale();

            // Step 3: check whether the transformed hit point is in the -1 to 1 range
            if ((LocalHitPoint.X >= -1.f) && (LocalHitPoint.X <= 1.f) && (LocalHitPoint.Y >= -1.f) && (LocalHitPoint.Y <= 1.f))
            {
                // Step 4: look up the hit texel and check whether it's transparent or opaque
                CVector2f TexCoord = (LocalHitPoint + CVector2f(1.f)) * 0.5f;
                TexCoord.X = -TexCoord.X + 1.f;
                float TexelAlpha = mpBillboard->ReadTexelAlpha(TexCoord);

                if (TexelAlpha < 0.25f)
                    Out.Hit = false;

                else
                {
                    // It's opaque... we have a hit!
                    Out.Hit = true;
                    Out.Distance = PlaneTest.second;
                }
            }

            else
                Out.Hit = false;
        }

        else
            Out.Hit = false;
    }

    return Out;
}

bool CScriptNode::AllowsRotate() const
{
    return (Template()->RotationType() == CScriptTemplate::eRotationEnabled);
}

bool CScriptNode::AllowsScale() const
{
    return (Template()->ScaleType() != CScriptTemplate::eScaleDisabled);
}

bool CScriptNode::IsVisible() const
{
    // Reimplementation of CSceneNode::IsVisible() to allow for layer and template visiblity to be taken into account
    return (mVisible && mpInstance->Layer()->IsVisible() && Template()->IsVisible());
}

CColor CScriptNode::TintColor(const SViewInfo &ViewInfo) const
{
    CColor BaseColor = CSceneNode::TintColor(ViewInfo);
    if (mpExtra) mpExtra->ModifyTintColor(BaseColor);
    return BaseColor;
}

void CScriptNode::LinksModified()
{
    if (mpExtra) mpExtra->LinksModified();
}

void CScriptNode::PropertyModified(IProperty *pProp)
{
    // Update volume
    if ( (pProp->Type() == eBoolProperty) || (pProp->Type() == eByteProperty) || (pProp->Type() == eShortProperty) ||
         (pProp->Type() == eLongProperty) || (pProp->Type() == eEnumProperty) )
    {
        mpInstance->EvaluateVolume();
        UpdatePreviewVolume();
    }

    // Update resources
    if (pProp->Type() == eCharacterProperty)
    {
        mpInstance->EvaluateDisplayModel();
        SetActiveModel(mpInstance->GetDisplayModel());
    }
    else if (pProp->Type() == eFileProperty)
    {
        CFileTemplate *pFile = static_cast<CFileTemplate*>(pProp->Template());

        if (pFile->AcceptsExtension("CMDL") || pFile->AcceptsExtension("ANCS") || pFile->AcceptsExtension("CHAR"))
        {
            mpInstance->EvaluateDisplayModel();
            SetActiveModel(mpInstance->GetDisplayModel());
        }
        else if (pFile->AcceptsExtension("TXTR"))
        {
            mpInstance->EvaluateBillboard();
            mpBillboard = mpInstance->GetBillboard();
        }
        else if (pFile->AcceptsExtension("DCLN"))
        {
            mpInstance->EvaluateCollisionModel();
            mpCollisionNode->SetCollision(mpInstance->GetCollision());
        }
    }

    // Update other editor properties
    if (mpInstance->IsEditorProperty(pProp))
    {
        CScriptTemplate *pTemplate = Template();

        if (pTemplate->HasName())
            SetName("[" + mpInstance->Template()->Name() + "] " + mpInstance->InstanceName());

        if (pTemplate->HasPosition())
            mPosition = mpInstance->Position();

        if (pTemplate->HasRotation())
            mRotation = CQuaternion::FromEuler(mpInstance->Rotation());

        if (pTemplate->HasScale())
            mScale = mpInstance->Scale();

        MarkTransformChanged();

        SetLightLayerIndex(mpLightParameters->LightLayerIndex());
    }

    // Update script extra
    if (mpExtra) mpExtra->PropertyModified(pProp);

    // Update game mode visibility
    if (pProp && pProp == mpInstance->ActiveProperty())
        TestGameModeVisibility();
}

void CScriptNode::UpdatePreviewVolume()
{
    EVolumeShape Shape = mpInstance->VolumeShape();
    TResPtr<CModel> pVolumeModel = nullptr;

    switch (Shape)
    {
    case eAxisAlignedBoxShape:
    case eBoxShape:
        pVolumeModel = gResCache.GetResource("../resources/VolumeBox.cmdl");
        break;

    case eEllipsoidShape:
        pVolumeModel = gResCache.GetResource("../resources/VolumeSphere.cmdl");
        break;

    case eCylinderShape:
        pVolumeModel = gResCache.GetResource("../resources/VolumeCylinder.cmdl");
        break;
    }

    mHasVolumePreview = (pVolumeModel != nullptr);

    if (mHasVolumePreview)
    {
        mpVolumePreviewNode->SetModel(pVolumeModel);
        mpVolumePreviewNode->SetScale(mpInstance->VolumeScale());
    }
}

void CScriptNode::GeneratePosition()
{
    if (!mHasValidPosition)
    {
        // Default to center of the active area; this is to preven recursion issues
        CTransform4f& AreaTransform = mpScene->ActiveArea()->Transform();
        mPosition = CVector3f(AreaTransform[0][3], AreaTransform[1][3], AreaTransform[2][3]);
        mHasValidPosition = true;
        MarkTransformChanged();

        // Ideal way to generate the position is to find a spot close to where it's being used.
        // To do this I check the location of the objects that this one is linked to.
        u32 NumLinks = mpInstance->NumLinks(eIncoming) + mpInstance->NumLinks(eOutgoing);

        // In the case of one link, apply an offset so the new position isn't the same place as the object it's linked to
        if (NumLinks == 1)
        {
            u32 LinkedID = (mpInstance->NumLinks(eIncoming) > 0 ? mpInstance->Link(eIncoming, 0)->SenderID() : mpInstance->Link(eOutgoing, 0)->ReceiverID());
            CScriptNode *pNode = mpScene->NodeForInstanceID(LinkedID);
            pNode->GeneratePosition();
            mPosition = pNode->AbsolutePosition();
            mPosition.Z += (pNode->AABox().Size().Z / 2.f);
            mPosition.Z += (AABox().Size().Z / 2.f);
            mPosition.Z += 2.f;
        }

        // For two or more links, average out the position of the connected objects.
        else if (NumLinks >= 2)
        {
            CVector3f NewPos = CVector3f::skZero;

            for (u32 iIn = 0; iIn < mpInstance->NumLinks(eIncoming); iIn++)
            {
                CScriptNode *pNode = mpScene->NodeForInstanceID(mpInstance->Link(eIncoming, iIn)->SenderID());

                if (pNode)
                {
                    pNode->GeneratePosition();
                    NewPos += pNode->AABox().Center();
                }
            }

            for (u32 iOut = 0; iOut < mpInstance->NumLinks(eOutgoing); iOut++)
            {
                CScriptNode *pNode = mpScene->NodeForInstanceID(mpInstance->Link(eOutgoing, iOut)->ReceiverID());

                if (pNode)
                {
                    pNode->GeneratePosition();
                    NewPos += pNode->AABox().Center();
                }
            }

            mPosition = NewPos / (float) NumLinks;
            mPosition.X += 2.f;
        }

        MarkTransformChanged();
    }
}

void CScriptNode::TestGameModeVisibility()
{
    // Don't render if we don't have an ingame model, or if this is the Prime series and the instance is not active.
    if ((Template()->Game() < eReturns && !mpInstance->IsActive()) || !mpInstance->HasInGameModel())
        mGameModeVisibility = eNotVisible;

    // If this is Returns, only render if the instance is active OR if it has a near visible activation.
    else
        mGameModeVisibility = (mpInstance->IsActive() || mpInstance->HasNearVisibleActivation()) ? eVisible : eNotVisible;
}

CColor CScriptNode::WireframeColor() const
{
    return CColor::Integral(12, 135, 194);
}

CScriptObject* CScriptNode::Instance() const
{
    return mpInstance;
}

CScriptTemplate* CScriptNode::Template() const
{
    return mpInstance->Template();
}

CScriptExtra* CScriptNode::Extra() const
{
    return mpExtra;
}

CModel* CScriptNode::ActiveModel() const
{
    return mpActiveModel;
}

bool CScriptNode::UsesModel() const
{
    return ((mpActiveModel != nullptr) || (mpBillboard == nullptr));
}

bool CScriptNode::HasPreviewVolume() const
{
    return mHasVolumePreview;
}

CAABox CScriptNode::PreviewVolumeAABox() const
{
    if (!mHasVolumePreview)
        return CAABox::skZero;
    else
        return mpVolumePreviewNode->AABox();
}

CVector2f CScriptNode::BillboardScale() const
{
    CVector2f Out = (Template()->ScaleType() == CScriptTemplate::eScaleEnabled ? AbsoluteScale().XZ() : CVector2f(1.f));
    return Out * 0.5f * Template()->PreviewScale();
}

// ************ PROTECTED ************
void CScriptNode::SetActiveModel(CModel *pModel)
{
    mpActiveModel = pModel;
    mLocalAABox = (pModel ? pModel->AABox() : CAABox::skOne);
    MarkTransformChanged();
}

void CScriptNode::CalculateTransform(CTransform4f& rOut) const
{
    CScriptTemplate *pTemp = Template();

    if (pTemp->ScaleType() != CScriptTemplate::eScaleDisabled)
    {
        CVector3f Scale = (HasPreviewVolume() ? CVector3f::skOne : AbsoluteScale());
        rOut.Scale(Scale * pTemp->PreviewScale());
    }

    if (pTemp->RotationType() == CScriptTemplate::eRotationEnabled)
        rOut.Rotate(AbsoluteRotation());

    rOut.Translate(AbsolutePosition());
}
