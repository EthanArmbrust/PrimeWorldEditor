#ifndef CSCENENODE_H
#define CSCENENODE_H

#include <Core/IRenderable.h>
#include "ENodeType.h"
#include <Common/CAABox.h>
#include <Common/CQuaternion.h>
#include <Common/CRay.h>
#include <Common/CRayCollisionTester.h>
#include <Common/CTransform4f.h>
#include <Common/CVector3f.h>
#include <Common/ETransformSpace.h>
#include <Common/types.h>
#include <Core/ERenderOptions.h>
#include <Resource/CLight.h>
#include <Resource/CGameArea.h>

class CRenderer;
class CSceneManager;

class CSceneNode : public IRenderable
{
private:
    mutable CTransform4f _mCachedTransform;
    mutable CAABox _mCachedAABox;
    mutable bool _mTransformDirty;

    bool _mInheritsPosition;
    bool _mInheritsRotation;
    bool _mInheritsScale;

protected:
    static u32 smNumNodes;
    TString mName;
    CSceneNode *mpParent;
    CSceneManager *mpScene;

    CVector3f mPosition;
    CQuaternion mRotation;
    CVector3f mScale;
    CAABox mLocalAABox;

    bool mMouseHovering;
    bool mSelected;
    bool mVisible;
    std::list<CSceneNode*> mChildren;

    u32 mLightLayerIndex;
    u32 mLightCount;
    CLight* mLights[8];
    CColor mAmbientColor;

public:
    explicit CSceneNode(CSceneManager *pScene, CSceneNode *pParent = 0);
    virtual ~CSceneNode();
    virtual ENodeType NodeType() = 0;
    virtual void AddToRenderer(CRenderer* /*pRenderer*/, const SViewInfo& /*ViewInfo*/) {}
    virtual void DrawSelection();
    virtual void RayAABoxIntersectTest(CRayCollisionTester& Tester, const SViewInfo& ViewInfo);
    virtual SRayIntersection RayNodeIntersectTest(const CRay& Ray, u32 AssetID, const SViewInfo& ViewInfo) = 0;
    virtual bool AllowsTranslate() const { return true; }
    virtual bool AllowsRotate() const { return true; }
    virtual bool AllowsScale() const { return true; }
    virtual bool IsVisible() const;
    virtual CColor TintColor(const SViewInfo& ViewInfo) const;
    virtual CColor WireframeColor() const;

    void Unparent();
    void RemoveChild(CSceneNode *pChild);
    void DeleteChildren();
    void SetInheritance(bool InheritPos, bool InheritRot, bool InheritScale);
    void LoadModelMatrix();
    void BuildLightList(CGameArea *pArea);
    void LoadLights(const SViewInfo& ViewInfo);
    void DrawBoundingBox() const;
    void AddSurfacesToRenderer(CRenderer *pRenderer, CModel *pModel, u32 MatSet, const SViewInfo& ViewInfo);

    // Transform
    void Translate(const CVector3f& translation, ETransformSpace transformSpace);
    void Rotate(const CQuaternion& rotation, ETransformSpace transformSpace);
    void Scale(const CVector3f& scale);
    const CTransform4f& Transform() const;
protected:
    void MarkTransformChanged() const;
    void ForceRecalculateTransform() const;
    virtual void CalculateTransform(CTransform4f& rOut) const;

public:
    // Getters
    TString Name() const;
    CSceneNode* Parent() const;
    CSceneManager* Scene() const;
    CVector3f LocalPosition() const;
    CVector3f AbsolutePosition() const;
    CQuaternion LocalRotation() const;
    CQuaternion AbsoluteRotation() const;
    CVector3f LocalScale() const;
    CVector3f AbsoluteScale() const;
    CAABox AABox() const;
    CVector3f CenterPoint() const;
    u32 LightLayerIndex() const;
    bool MarkedVisible() const;
    bool IsMouseHovering() const;
    bool IsSelected() const;
    bool InheritsPosition() const;
    bool InheritsRotation() const;
    bool InheritsScale() const;

    // Setters
    void SetName(const TString& Name);
    void SetPosition(const CVector3f& position);
    void SetRotation(const CQuaternion& rotation);
    void SetRotation(const CVector3f& rotEuler);
    void SetScale(const CVector3f& scale);
    void SetLightLayerIndex(u32 index);
    void SetMouseHovering(bool Hovering);
    void SetSelected(bool Selected);
    void SetVisible(bool Visible);

    // Static
    static int NumNodes();
    static CColor skSelectionTint;
};

// ************ INLINE FUNCTIONS ************
inline int CSceneNode::NumNodes()
{
    return smNumNodes;
}

#endif // CSCENENODE_H