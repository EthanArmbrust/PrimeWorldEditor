#ifndef CMASTERTEMPLATE_H
#define CMASTERTEMPLATE_H

#include "CScriptTemplate.h"
#include "Core/Resource/EGame.h"
#include <Common/types.h>
#include <map>

class CMasterTemplate
{
    friend class CTemplateLoader;
    friend class CTemplateWriter;

    EGame mGame;
    TString mGameName;
    TString mSourceFile;
    u32 mVersion;
    bool mFullyLoaded;

    std::vector<TString> mGameVersions;
    std::map<u32, CScriptTemplate*> mTemplates;
    std::map<u32, TString> mStates;
    std::map<u32, TString> mMessages;


    static std::map<EGame, CMasterTemplate*> smMasterMap;
    static std::map<u32, TString> smPropertyNames;
    static u32 smGameListVersion;

public:
    CMasterTemplate();
    ~CMasterTemplate();
    EGame GetGame();
    u32 NumGameVersions();
    u32 GetGameVersion(TString VersionName);
    u32 NumScriptTemplates();
    u32 NumStates();
    u32 NumMessages();
    CScriptTemplate* TemplateByID(u32 ObjectID);
    CScriptTemplate* TemplateByID(const CFourCC& ObjectID);
    CScriptTemplate* TemplateByIndex(u32 Index);
    TString StateByID(u32 StateID);
    TString StateByID(const CFourCC& StateID);
    TString StateByIndex(u32 Index);
    TString MessageByID(u32 MessageID);
    TString MessageByID(const CFourCC& MessageID);
    TString MessageByIndex(u32 Index);
    bool IsLoadedSuccessfully();

    static CMasterTemplate* GetMasterForGame(EGame Game);
    static std::list<CMasterTemplate*> GetMasterList();
    static TString GetPropertyName(u32 PropertyID);
};

// ************ INLINE ************
inline u32 CMasterTemplate::NumScriptTemplates() {
    return mTemplates.size();
}

inline u32 CMasterTemplate::NumStates() {
    return mStates.size();
}

inline u32 CMasterTemplate::NumMessages() {
    return mMessages.size();
}

#endif // CMASTERTEMPLATE_H
