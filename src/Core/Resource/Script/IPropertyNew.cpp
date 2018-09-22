#include "IPropertyNew.h"
#include "Property/CAssetProperty.h"
#include "Property/CArrayProperty.h"
#include "Property/CEnumProperty.h"
#include "Property/CFlagsProperty.h"
#include "Property/CPointerProperty.h"

#include "Core/Resource/Script/CMasterTemplate.h"
#include "Core/Resource/Script/CScriptTemplate.h"

/** IPropertyNew */
IPropertyNew::IPropertyNew(EGame Game)
    : mpParent( nullptr )
    , mpPointerParent( nullptr )
    , mpArchetype( nullptr )
    , mGame( Game )
    , mpScriptTemplate( nullptr )
    , mOffset( -1 )
    , mID( -1 )
    , mCookPreference( ECookPreferenceNew::Default )
    , mMinVersion( 0.0f )
    , mMaxVersion( FLT_MAX )
{}

void IPropertyNew::_ClearChildren()
{
    for (int ChildIdx = 0; ChildIdx < mChildren.size(); ChildIdx++)
        delete mChildren[ChildIdx];

    mChildren.clear();
}

IPropertyNew::~IPropertyNew()
{
    // Remove from archetype
    if( mpArchetype != nullptr )
    {
        NBasics::VectorRemoveOne(mpArchetype->mSubInstances, this);
    }

    // If this is an archetype, all our sub-instances should have destructed first.
    if( IsArchetype() )
    {
        ASSERT(mSubInstances.empty());
    }

    // Delete children
    _ClearChildren();
}

const char* IPropertyNew::HashableTypeName() const
{
    return PropEnumToHashableTypeName( Type() );
}

void* IPropertyNew::GetChildDataPointer(void* pPropertyData) const
{
    return pPropertyData;
}

void IPropertyNew::Serialize(IArchive& rArc)
{
    // Always serialize ID first! ID is always required (except for root properties, which have an ID of 0xFFFFFFFF)
    // because they are needed to look up the correct property to apply parameter overrides to.
    rArc << SerialParameter("ID", mID, SH_HexDisplay | SH_Attribute | SH_Optional, (u32) 0xFFFFFFFF);

    // Now we can serialize the archetype reference and initialize if needed
    if ( ((mpArchetype && mpArchetype->IsRootParent()) || rArc.IsReader()) && rArc.CanSkipParameters() )
    {
        TString ArchetypeName = (mpArchetype ? mpArchetype->Name() : "");
        rArc << SerialParameter("Archetype", ArchetypeName, SH_Attribute);

        if (rArc.IsReader() && !ArchetypeName.IsEmpty())
        {
            CMasterTemplate* pMaster = CMasterTemplate::MasterForGame( Game() );
            IPropertyNew* pArchetype = pMaster->FindPropertyArchetype(ArchetypeName);

            // The archetype must exist, or else the template file is malformed.
            ASSERT(pArchetype != nullptr);

            InitFromArchetype(pArchetype);
        }
    }

    // In MP1, the game data does not use property IDs, so we serialize the name directly.
    // In MP2 and on, property names are looked up based on the property ID via the property name map.
    // Exceptions: Properties that are not in the name map still need to serialize their names.
    // This includes root-level properties, and properties of atomic structs.
    //
    // We can't currently tell if this property is atomic, as the flag hasn't been serialized and the parent
    // hasn't been set, but atomic sub-properties don't use hash IDs, so we can do a pseudo-check against the ID.
    if (rArc.Game() <= ePrime || IsRootParent() || mID <= 0xFF)
    {
        rArc << SerialParameter("Name", mName, mpArchetype ? SH_Optional : 0, mpArchetype ? mpArchetype->mName : "");
    }

    rArc << SerialParameter("Description",      mDescription,       SH_Optional, mpArchetype ? mpArchetype->mDescription : "")
         << SerialParameter("CookPreference",   mCookPreference,    SH_Optional, mpArchetype ? mpArchetype->mCookPreference : ECookPreferenceNew::Default)
         << SerialParameter("MinVersion",       mMinVersion,        SH_Optional, mpArchetype ? mpArchetype->mMinVersion : 0.f)
         << SerialParameter("MaxVersion",       mMaxVersion,        SH_Optional, mpArchetype ? mpArchetype->mMaxVersion : FLT_MAX)
         << SerialParameter("Suffix",           mSuffix,            SH_Optional, mpArchetype ? mpArchetype->mSuffix : "");

    // Children don't get serialized for most property types
}

void IPropertyNew::InitFromArchetype(IPropertyNew* pOther)
{
    //@todo maybe somehow use Serialize for this instead?
    mpArchetype = pOther;
    mFlags = pOther->mFlags & EPropertyFlag::ArchetypeCopyFlags;
    mName = pOther->mName;
    mDescription = pOther->mDescription;
    mSuffix = pOther->mSuffix;
    mCookPreference = pOther->mCookPreference;
    mMinVersion = pOther->mMinVersion;
    mMaxVersion = pOther->mMaxVersion;

    // Copy ID only if our existing ID is not valid.
    if (mID == 0xFFFFFFFF)
    {
        mID = pOther->mID;
    }
}

bool IPropertyNew::ShouldSerialize() const
{
    return mpArchetype == nullptr ||
           mName != mpArchetype->mName ||
           mDescription != mpArchetype->mDescription ||
           mSuffix != mpArchetype->mSuffix ||
           mCookPreference != mpArchetype->mCookPreference ||
           mMinVersion != mpArchetype->mMinVersion ||
           mMaxVersion != mpArchetype->mMaxVersion;
}

TString IPropertyNew::GetTemplateFileName()
{
    if (mpScriptTemplate)
    {
        return mpScriptTemplate->SourceFile();
    }
    else if (IsArchetype())
    {
        IPropertyNew* pRootParent = RootParent();
        ASSERT(pRootParent != this);
        return pRootParent->GetTemplateFileName();
    }
    else
    {
        return mpArchetype->GetTemplateFileName();
    }
}

void IPropertyNew::Initialize(IPropertyNew* pInParent, CScriptTemplate* pInTemplate, u32 InOffset)
{
    // Make sure we only get initialized once.
    ASSERT( (mFlags & EPropertyFlag::IsInitialized) == 0 );
    mFlags |= EPropertyFlag::IsInitialized;

    mpParent = pInParent;
    mOffset = InOffset;
    mpScriptTemplate = pInTemplate;

    // Look up property name if needed.
    if (Game() >= eEchoesDemo && !IsRootParent() && !IsIntrinsic() && !mpParent->IsAtomic())
    {
        mName = CMasterTemplate::PropertyName(mID);
    }

    // Set any fields dependent on the parent...
    if (mpParent)
    {
        mFlags |= mpParent->mFlags & EPropertyFlag::InheritableFlags;

        if (mpParent->IsPointerType())
        {
            mpPointerParent = mpParent;
        }
        else
        {
            mpPointerParent = mpParent->mpPointerParent;
        }
    }

    // Allow subclasses to handle any initialization tasks
    PostInitialize();

    // Now, route initialization to any child properties...
    u32 ChildOffset = mOffset;

    for (int ChildIdx = 0; ChildIdx < mChildren.size(); ChildIdx++)
    {
        IPropertyNew* pChild = mChildren[ChildIdx];

        // update offset and round up to the child's alignment
        if (ChildIdx > 0)
        {
            ChildOffset += mChildren[ChildIdx-1]->DataSize();
        }
        ChildOffset = ALIGN(ChildOffset, pChild->DataAlignment());

        // Don't call Initialize on intrinsic children as they have already been initialized.
        if (!pChild->IsIntrinsic())
        {
            pChild->Initialize(this, pInTemplate, ChildOffset);
        }
    }
}

void* IPropertyNew::RawValuePtr(void* pData) const
{
    // For array archetypes, the caller needs to provide the pointer to the correct array item
    // Array archetypes can't store their index in the array so it's impossible to determine the correct pointer.
    void* pBasePtr = (mpPointerParent && !IsArrayArchetype() ? mpPointerParent->GetChildDataPointer(pData) : pData);
    void* pValuePtr = ((char*)pBasePtr + mOffset);
    return pValuePtr;
}

IPropertyNew* IPropertyNew::ChildByID(u32 ID) const
{
    for (u32 ChildIdx = 0; ChildIdx < mChildren.size(); ChildIdx++)
    {
        if (mChildren[ChildIdx]->mID == ID)
            return mChildren[ChildIdx];
    }

    return nullptr;
}

IPropertyNew* IPropertyNew::ChildByIDString(const TIDString& rkIdString)
{
    // String must contain at least one ID!
    // some ID strings are formatted with 8 characters and some with 2 (plus the beginning "0x")
    ASSERT(rkIdString.Size() >= 4);

    u32 IDEndPos = rkIdString.IndexOf(':');
    u32 NextChildID = -1;

    if (IDEndPos == -1)
        NextChildID = rkIdString.ToInt32();
    else
        NextChildID = rkIdString.SubString(2, IDEndPos - 2).ToInt32();

    if (NextChildID == 0xFFFFFFFF)
    {
        return nullptr;
    }

    IPropertyNew* pNextChild = ChildByID(NextChildID);

    // Check if we need to recurse
    if (IDEndPos != -1)
    {
        return pNextChild->ChildByIDString(rkIdString.ChopFront(IDEndPos + 1));
    }
    else
    {
        return pNextChild;
    }
}

bool IPropertyNew::ShouldCook(void*pPropertyData) const
{
    switch (mCookPreference)
    {
    case ECookPreferenceNew::Always:
        return true;

    case ECookPreferenceNew::Never:
        return false;

    default:
        return (Game() < eReturns ? true : !MatchesDefault(pPropertyData));
    }
}

void IPropertyNew::SetName(const TString& rkNewName)
{
    mName = rkNewName;
    mFlags.ClearFlag(EPropertyFlag::HasCachedNameCheck);
}

void IPropertyNew::SetDescription(const TString& rkNewDescription)
{
    mDescription = rkNewDescription;
}

void IPropertyNew::SetSuffix(const TString& rkNewSuffix)
{
    mSuffix = rkNewSuffix;
}

void IPropertyNew::SetPropertyFlags(FPropertyFlags FlagsToSet)
{
    mFlags |= FlagsToSet;
}

bool IPropertyNew::HasAccurateName()
{
    // Exceptions for the three hardcoded 4CC property IDs
    if (mID == FOURCC('XFRM') || mID == FOURCC('INAM') || mID == FOURCC('ACTV'))
        return true;

    // Children of atomic properties defer to parents. Intrinsic properties also defer to parents.
    if ( (mpParent && mpParent->IsAtomic()) || IsIntrinsic() )
    {
        if (mpParent)
            return mpParent->HasAccurateName();
        else
            return true;
    }

    // For everything else, hash the property name and check if it is a match for the property ID
    if (!mFlags.HasFlag(EPropertyFlag::HasCachedNameCheck))
    {
        CCRC32 Hash;
        Hash.Hash(*mName);
        Hash.Hash(HashableTypeName());
        u32 GeneratedID = Hash.Digest();

        if (GeneratedID == mID)
            mFlags.SetFlag( EPropertyFlag::HasCorrectPropertyName );
        else
            mFlags.ClearFlag( EPropertyFlag::HasCorrectPropertyName );

        mFlags.SetFlag(EPropertyFlag::HasCachedNameCheck);
    }

    return mFlags.HasFlag( EPropertyFlag::HasCorrectPropertyName );
}

/** IPropertyNew Accessors */
EGame IPropertyNew::Game() const
{
    return mGame;
}

IPropertyNew* IPropertyNew::Create(EPropertyTypeNew Type,
                                   EGame Game)
{
    IPropertyNew* pOut = nullptr;

    switch (Type)
    {
    case EPropertyTypeNew::Bool:            pOut = new CBoolProperty(Game);             break;
    case EPropertyTypeNew::Byte:            pOut = new CByteProperty(Game);             break;
    case EPropertyTypeNew::Short:           pOut = new CShortProperty(Game);            break;
    case EPropertyTypeNew::Int:             pOut = new CIntProperty(Game);              break;
    case EPropertyTypeNew::Float:           pOut = new CFloatProperty(Game);            break;
    case EPropertyTypeNew::Choice:          pOut = new CChoiceProperty(Game);           break;
    case EPropertyTypeNew::Enum:            pOut = new CEnumProperty(Game);             break;
    case EPropertyTypeNew::Flags:           pOut = new CFlagsProperty(Game);            break;
    case EPropertyTypeNew::String:          pOut = new CStringProperty(Game);           break;
    case EPropertyTypeNew::Vector:          pOut = new CVectorProperty(Game);           break;
    case EPropertyTypeNew::Color:           pOut = new CColorProperty(Game);            break;
    case EPropertyTypeNew::Asset:           pOut = new CAssetProperty(Game);            break;
    case EPropertyTypeNew::Sound:           pOut = new CSoundProperty(Game);            break;
    case EPropertyTypeNew::Animation:       pOut = new CAnimationProperty(Game);        break;
    case EPropertyTypeNew::AnimationSet:    pOut = new CAnimationSetProperty(Game);     break;
    case EPropertyTypeNew::Sequence:        pOut = new CSequenceProperty(Game);         break;
    case EPropertyTypeNew::Spline:          pOut = new CSplineProperty(Game);           break;
    case EPropertyTypeNew::Guid:            pOut = new CGuidProperty(Game);             break;
    case EPropertyTypeNew::Pointer:         pOut = new CPointerProperty(Game);          break;
    case EPropertyTypeNew::Struct:          pOut = new CStructPropertyNew(Game);        break;
    case EPropertyTypeNew::Array:           pOut = new CArrayProperty(Game);            break;
    }

    // If this assertion fails, then there is an unhandled type!
    ASSERT(pOut != nullptr);
    return pOut;
}

IPropertyNew* IPropertyNew::CreateCopy(IPropertyNew* pArchetype)
{
    IPropertyNew* pOut = Create(pArchetype->Type(), pArchetype->mGame);
    pOut->InitFromArchetype(pArchetype);
    pArchetype->mSubInstances.push_back(pOut);
    return pOut;
}

IPropertyNew* IPropertyNew::CreateIntrinsic(EPropertyTypeNew Type,
                                            EGame Game,
                                            u32 Offset,
                                            const TString& rkName)
{
    IPropertyNew* pOut = Create(Type, Game);
    pOut->mFlags |= EPropertyFlag::IsIntrinsic;
    pOut->SetName(rkName);
    pOut->Initialize(nullptr, nullptr, Offset);
    return pOut;
}

IPropertyNew* IPropertyNew::CreateIntrinsic(EPropertyTypeNew Type,
                                            IPropertyNew* pParent,
                                            u32 Offset,
                                            const TString& rkName)
{
    // pParent should always be valid.
    // If you are creating a root property, call the other overload takes an EGame instead of a parent.
    ASSERT(pParent != nullptr);

    IPropertyNew* pOut = Create(Type, pParent->mGame);
    pOut->mFlags |= EPropertyFlag::IsIntrinsic;
    pOut->SetName(rkName);
    pOut->Initialize(pParent, nullptr, Offset);
    pParent->mChildren.push_back(pOut);
    return pOut;
}

IPropertyNew* IPropertyNew::ArchiveConstructor(EPropertyTypeNew Type,
                                               const IArchive& Arc)
{
    return Create(Type, Arc.Game());
}
