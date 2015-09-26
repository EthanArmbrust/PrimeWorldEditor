#ifndef CTEMPLATEWRITER_H
#define CTEMPLATEWRITER_H

#include "../script/CMasterTemplate.h"
#include "../script/CScriptTemplate.h"

class CTemplateWriter
{
    CTemplateWriter();

public:
    static void SaveAllTemplates();
    static void SaveGameTemplates(CMasterTemplate *pMaster, const std::string& dir);
    static void SavePropertyList(CMasterTemplate *pMaster, const std::string& dir);
    static void SaveScriptTemplate(CScriptTemplate *pTemp, const std::string& dir);
    static void SaveStructTemplate(CStructTemplate *pTemp, CMasterTemplate *pMaster, const std::string& dir);
    static void SaveProperties(tinyxml2::XMLDocument *pDoc, tinyxml2::XMLElement *pParent, CStructTemplate *pTemp, CMasterTemplate *pMaster, const std::string& dir);
    // todo: save enum templates
};

#endif // CTEMPLATEWRITER_H