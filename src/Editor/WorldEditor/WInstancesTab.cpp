#include "WInstancesTab.h"
#include "ui_WInstancesTab.h"

#include "CWorldEditor.h"
#include <Core/Resource/Script/CScriptLayer.h>
#include <Core/Scene/CScene.h>
#include <Core/Scene/CSceneIterator.h>

WInstancesTab::WInstancesTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WInstancesTab)
{
    ui->setupUi(this);

    mpEditor = nullptr;
    mpLayersModel = new CInstancesModel(this);
    mpLayersModel->SetModelType(CInstancesModel::eLayers);
    mpTypesModel = new CInstancesModel(this);
    mpTypesModel->SetModelType(CInstancesModel::eTypes);
    mLayersProxyModel.setSourceModel(mpLayersModel);
    mTypesProxyModel.setSourceModel(mpTypesModel);

    int ColWidth = ui->LayersTreeView->width() * 0.29;

    ui->LayersTreeView->setModel(&mLayersProxyModel);
    ui->LayersTreeView->resizeColumnToContents(2);
    ui->LayersTreeView->header()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->LayersTreeView->header()->resizeSection(0, ColWidth);
    ui->LayersTreeView->header()->resizeSection(1, ColWidth);
    ui->LayersTreeView->header()->setSortIndicator(0, Qt::AscendingOrder);

    ui->TypesTreeView->setModel(&mTypesProxyModel);
    ui->TypesTreeView->resizeColumnToContents(2);
    ui->TypesTreeView->header()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->TypesTreeView->header()->resizeSection(0, ColWidth);
    ui->TypesTreeView->header()->resizeSection(1, ColWidth);
    ui->TypesTreeView->header()->setSortIndicator(0, Qt::AscendingOrder);

    connect(ui->LayersTreeView, SIGNAL(clicked(QModelIndex)), this, SLOT(OnTreeClick(QModelIndex)));
    connect(ui->LayersTreeView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(OnTreeDoubleClick(QModelIndex)));
    connect(ui->LayersTreeView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnTreeContextMenu(QPoint)));
    connect(ui->TypesTreeView, SIGNAL(clicked(QModelIndex)), this, SLOT(OnTreeClick(QModelIndex)));
    connect(ui->TypesTreeView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(OnTreeDoubleClick(QModelIndex)));
    connect(ui->TypesTreeView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnTreeContextMenu(QPoint)));

    // Create context menu
    mpHideInstance = new QAction("Hide instance", this);
    mpHideType = new QAction("HideType", this);
    mpHideAllTypes = new QAction("Hide all types", this);
    mpHideAllExceptType = new QAction("HideAllButType", this);
    mpSeparator = new QAction(this);
    mpSeparator->setSeparator(true);
    mpUnhideAllTypes = new QAction("UnhideAllTypes", this);
    mpUnhideAll = new QAction("Unhide all", this);

    QList<QAction*> ActionList;
    ActionList << mpHideInstance << mpHideType << mpHideAllTypes << mpHideAllExceptType << mpSeparator
               << mpUnhideAllTypes << mpUnhideAll;

    mpTreeContextMenu = new QMenu(this);
    mpTreeContextMenu->addActions(ActionList);

    connect(mpHideInstance, SIGNAL(triggered()), this, SLOT(OnHideInstanceAction()));
    connect(mpHideType, SIGNAL(triggered()), this, SLOT(OnHideTypeAction()));
    connect(mpHideAllTypes, SIGNAL(triggered()), this, SLOT(OnHideAllTypesAction()));
    connect(mpHideAllExceptType, SIGNAL(triggered()), this, SLOT(OnHideAllExceptTypeAction()));
    connect(mpUnhideAllTypes, SIGNAL(triggered()), this, SLOT(OnUnhideAllTypes()));
    connect(mpUnhideAll, SIGNAL(triggered()), this, SLOT(OnUnhideAll()));
}

WInstancesTab::~WInstancesTab()
{
    delete ui;
}

void WInstancesTab::SetEditor(CWorldEditor *pEditor, CScene *pScene)
{
    mpEditor = pEditor;
    mpScene = pScene;
    mpTypesModel->SetEditor(pEditor);
    mpLayersModel->SetEditor(pEditor);
}

void WInstancesTab::SetMaster(CMasterTemplate *pMaster)
{
    mpTypesModel->SetMaster(pMaster);
    ExpandTopLevelItems();
}

void WInstancesTab::SetArea(CGameArea *pArea)
{
    mpLayersModel->SetArea(pArea);
    ExpandTopLevelItems();
}

// ************ PRIVATE SLOTS ************
void WInstancesTab::OnTreeClick(QModelIndex Index)
{
    // Single click is used to process show/hide events
    QModelIndex SourceIndex = (ui->TabWidget->currentIndex() == 0 ? mLayersProxyModel.mapToSource(Index) : mTypesProxyModel.mapToSource(Index));

    if (SourceIndex.column() == 2)
    {
        // Show/Hide Instance
        if (mpTypesModel->IndexType(SourceIndex) == CInstancesModel::eInstanceIndex)
        {
            CScriptObject *pObj = mpTypesModel->IndexObject(SourceIndex);

            if (pObj)
            {
                CScriptNode *pNode = mpScene->NodeForObject(pObj);

                if (pNode)
                    pNode->SetVisible(!pNode->IsVisible());
            }

        }

        // Show/Hide Object Type
        else if (mpTypesModel->IndexType(SourceIndex) == CInstancesModel::eObjectTypeIndex)
        {
            if (sender() == ui->LayersTreeView)
            {
                CScriptLayer *pLayer = mpLayersModel->IndexLayer(SourceIndex);
                pLayer->SetVisible(!pLayer->IsVisible());
            }

            else if (sender() == ui->TypesTreeView)
            {
                CScriptTemplate *pTmp = mpTypesModel->IndexTemplate(SourceIndex);
                pTmp->SetVisible(!pTmp->IsVisible());
            }
        }

        static_cast<QTreeView*>(sender())->update(Index);
    }
}

void WInstancesTab::OnTreeDoubleClick(QModelIndex Index)
{
    QModelIndex SourceIndex = (ui->TabWidget->currentIndex() == 0 ? mLayersProxyModel.mapToSource(Index) : mTypesProxyModel.mapToSource(Index));;
    CInstancesModel::EIndexType IndexType = mpTypesModel->IndexType(SourceIndex);

    if ((mpEditor) && (IndexType == CInstancesModel::eInstanceIndex))
    {
        CInstancesModel::ENodeType NodeType = mpTypesModel->IndexNodeType(SourceIndex);
        CSceneNode *pSelectedNode = nullptr;

        if (NodeType == CInstancesModel::eScriptType)
            pSelectedNode = mpScene->NodeForObject( static_cast<CScriptObject*>(SourceIndex.internalPointer()) );

        if (pSelectedNode)
        {
            mpEditor->ClearSelection();
            mpEditor->SelectNode(pSelectedNode);
        }
    }
}

void WInstancesTab::OnTreeContextMenu(QPoint Pos)
{
    bool IsLayers = (sender() == ui->LayersTreeView);

    QModelIndex Index = (IsLayers ? ui->LayersTreeView->indexAt(Pos) : ui->TypesTreeView->indexAt(Pos));
    mMenuIndex = (IsLayers ? mLayersProxyModel.mapToSource(Index) : mTypesProxyModel.mapToSource(Index));

    // Determine type
    mMenuIndexType = (IsLayers ? mpLayersModel->IndexType(mMenuIndex) : mpTypesModel->IndexType(mMenuIndex));

    CScriptObject *pObject = nullptr;
    mpMenuObject = nullptr;
    mpMenuLayer = nullptr;
    mpMenuTemplate = nullptr;

    if (mMenuIndexType == CInstancesModel::eObjectTypeIndex)
    {
        pObject = nullptr;
        mpMenuObject = nullptr;

        if (IsLayers)
            mpMenuLayer = mpLayersModel->IndexLayer(mMenuIndex);
        else
            mpMenuTemplate = mpTypesModel->IndexTemplate(mMenuIndex);
    }

    else if (mMenuIndexType == CInstancesModel::eInstanceIndex)
    {
        pObject = ( IsLayers ? mpLayersModel->IndexObject(mMenuIndex) : mpTypesModel->IndexObject(mMenuIndex) );
        mpMenuObject = mpScene->NodeForObject(pObject);

        if (IsLayers)
            mpMenuLayer = pObject->Layer();
        else
            mpMenuTemplate = pObject->Template();
    }

    // Set visibility and text
    if (pObject)
    {
        QString Hide = mpMenuObject->MarkedVisible() ? "Hide" : "Unhide";
        mpHideInstance->setText(QString("%1 instance").arg(Hide));
        mpHideInstance->setVisible(true);
    }

    else
    {
        mpHideInstance->setVisible(false);
    }

    if (mpMenuLayer)
    {
        QString Hide = mpMenuLayer->IsVisible() ? "Hide" : "Unhide";
        mpHideType->setText(QString("%1 layer %2").arg(Hide).arg(TO_QSTRING(mpMenuLayer->Name())));
        mpHideType->setVisible(true);

        mpHideAllExceptType->setText(QString("Hide all layers but %1").arg(TO_QSTRING(mpMenuLayer->Name())));
        mpHideAllExceptType->setVisible(true);
    }

    else if (mpMenuTemplate)
    {
        QString Hide = mpMenuTemplate->IsVisible() ? "Hide" : "Unhide";
        mpHideType->setText(QString("%1 all %2 objects").arg(Hide).arg(TO_QSTRING(mpMenuTemplate->Name())));
        mpHideType->setVisible(true);

        mpHideAllExceptType->setText(QString("Hide all types but %1").arg(TO_QSTRING(mpMenuTemplate->Name())));
        mpHideAllExceptType->setVisible(true);
    }

    else
    {
        mpHideType->setVisible(false);
        mpHideAllExceptType->setVisible(false);
    }

    mpHideAllTypes->setText(QString("Hide all %1").arg(IsLayers ? "layers" : "types"));
    mpUnhideAllTypes->setText(QString("Unhide all %1").arg(IsLayers ? "layers" : "types"));

    QPoint GlobalPos = static_cast<QTreeView*>(sender())->viewport()->mapToGlobal(Pos);
    mpTreeContextMenu->exec(GlobalPos);
}

void WInstancesTab::OnHideInstanceAction()
{
    bool IsLayers = (ui->TabWidget->currentIndex() == 0);
    mpMenuObject->SetVisible(mpMenuObject->MarkedVisible() ? false : true);

    if (IsLayers)
        mpLayersModel->dataChanged(mMenuIndex, mMenuIndex);
    else
        mpTypesModel->dataChanged(mMenuIndex, mMenuIndex);
}

void WInstancesTab::OnHideTypeAction()
{
    bool IsLayers = (ui->TabWidget->currentIndex() == 0);
    QModelIndex TypeIndex = (mMenuIndexType == CInstancesModel::eInstanceIndex ? mMenuIndex.parent() : mMenuIndex);

    if (IsLayers)
    {
        mpMenuLayer->SetVisible(mpMenuLayer->IsVisible() ? false : true);
        mpLayersModel->dataChanged(TypeIndex, TypeIndex);
    }

    else
    {
        mpMenuTemplate->SetVisible(mpMenuTemplate->IsVisible() ? false : true);
        mpTypesModel->dataChanged(TypeIndex, TypeIndex);
    }
}

void WInstancesTab::OnHideAllTypesAction()
{
    bool IsLayers = (ui->TabWidget->currentIndex() == 0);
    CInstancesModel *pModel = (IsLayers ? mpLayersModel : mpTypesModel);

    QModelIndex BaseIndex = pModel->index(0, 0);

    for (int iIdx = 0; iIdx < pModel->rowCount(BaseIndex); iIdx++)
    {
        QModelIndex Index = pModel->index(iIdx, 0, BaseIndex);

        if (IsLayers)
        {
            CScriptLayer *pLayer = pModel->IndexLayer(Index);
            pLayer->SetVisible(false);
        }

        else
        {
            CScriptTemplate *pTemplate = pModel->IndexTemplate(Index);
            pTemplate->SetVisible(false);
        }
    }

    pModel->dataChanged(pModel->index(0, 2, BaseIndex), pModel->index(pModel->rowCount(BaseIndex) - 1, 2, BaseIndex));
}

void WInstancesTab::OnHideAllExceptTypeAction()
{
    bool IsLayers = (ui->TabWidget->currentIndex() == 0);
    QModelIndex TypeIndex = (mMenuIndexType == CInstancesModel::eInstanceIndex ? mMenuIndex.parent() : mMenuIndex);
    QModelIndex TypeParent = TypeIndex.parent();

    if (IsLayers)
    {
        CGameArea *pArea = mpEditor->ActiveArea();

        for (u32 iLyr = 0; iLyr < pArea->GetScriptLayerCount(); iLyr++)
        {
            CScriptLayer *pLayer = pArea->GetScriptLayer(iLyr);
            pLayer->SetVisible( pLayer == mpMenuLayer ? true : false );
        }

        mpLayersModel->dataChanged( mpLayersModel->index(0, 2, TypeParent), mpLayersModel->index(mpLayersModel->rowCount(TypeParent) - 1, 2, TypeParent) );
    }

    else
    {
        EGame Game = mpEditor->ActiveArea()->Version();
        CMasterTemplate *pMaster = CMasterTemplate::GetMasterForGame(Game);

        for (u32 iTemp = 0; iTemp < pMaster->NumScriptTemplates(); iTemp++)
        {
            CScriptTemplate *pTemplate = pMaster->TemplateByIndex(iTemp);
            pTemplate->SetVisible( pTemplate == mpMenuTemplate ? true : false );
        }

        mpTypesModel->dataChanged( mpTypesModel->index(0, 2, TypeParent), mpTypesModel->index(mpTypesModel->rowCount(TypeParent) - 1, 2, TypeParent) );
    }
}

void WInstancesTab::OnUnhideAllTypes()
{
    bool IsLayers = (ui->TabWidget->currentIndex() == 0);
    QModelIndex TypeIndex = (mMenuIndexType == CInstancesModel::eInstanceIndex ? mMenuIndex.parent() : mMenuIndex);
    QModelIndex TypeParent = TypeIndex.parent();

    if (IsLayers)
    {
        CGameArea *pArea = mpEditor->ActiveArea();

        for (u32 iLyr = 0; iLyr < pArea->GetScriptLayerCount(); iLyr++)
            pArea->GetScriptLayer(iLyr)->SetVisible(true);

        mpLayersModel->dataChanged( mpLayersModel->index(0, 2, TypeParent), mpLayersModel->index(mpLayersModel->rowCount(TypeParent) - 1, 2, TypeParent) );
    }

    else
    {
        EGame Game = mpEditor->ActiveArea()->Version();
        CMasterTemplate *pMaster = CMasterTemplate::GetMasterForGame(Game);

        for (u32 iTemp = 0; iTemp < pMaster->NumScriptTemplates(); iTemp++)
            pMaster->TemplateByIndex(iTemp)->SetVisible(true);

        mpTypesModel->dataChanged( mpTypesModel->index(0, 2, TypeParent), mpTypesModel->index(mpTypesModel->rowCount(TypeParent) - 1, 2, TypeParent) );
    }
}

void WInstancesTab::OnUnhideAll()
{
    // Unhide instances
    for (CSceneIterator It(mpScene, eScriptNode, true); !It.DoneIterating(); ++It)
        It->SetVisible(true);

    // Unhide layers
    QModelIndex LayersRoot = mpLayersModel->index(0, 0, QModelIndex()).child(0, 0);

    if (LayersRoot.isValid())
    {
        CGameArea *pArea = mpEditor->ActiveArea();

        for (u32 iLyr = 0; iLyr < pArea->GetScriptLayerCount(); iLyr++)
            pArea->GetScriptLayer(iLyr)->SetVisible(true);

        mpLayersModel->dataChanged( mpLayersModel->index(0, 2, LayersRoot), mpLayersModel->index(mpLayersModel->rowCount(LayersRoot) - 1, 2, LayersRoot) );
    }

    // Unhide types
    QModelIndex TypesRoot = mpTypesModel->index(0, 0, QModelIndex()).child(0, 0);

    if (TypesRoot.isValid())
    {
        EGame Game = mpEditor->ActiveArea()->Version();
        CMasterTemplate *pMaster = CMasterTemplate::GetMasterForGame(Game);

        for (u32 iTemp = 0; iTemp < pMaster->NumScriptTemplates(); iTemp++)
            pMaster->TemplateByIndex(iTemp)->SetVisible(true);

        mpTypesModel->dataChanged( mpTypesModel->index(0, 2, TypesRoot), mpTypesModel->index(mpTypesModel->rowCount(TypesRoot) - 1, 2, TypesRoot) );
    }

    // Emit data changed on all instances
    for (u32 iModel = 0; iModel < 2; iModel++)
    {
        CInstancesModel *pModel = (iModel == 0 ? mpLayersModel : mpTypesModel);

        QModelIndex Base = pModel->index(0, 0);
        u32 NumRows = pModel->rowCount(Base);

        for (u32 iRow = 0; iRow < NumRows; iRow++)
        {
            QModelIndex RowIndex = pModel->index(iRow, 2, Base);
            pModel->dataChanged( pModel->index(0, 2, RowIndex), pModel->index(pModel->rowCount(RowIndex) - 1, 2, RowIndex) );
        }
    }

    OnUnhideAllTypes();
}

void WInstancesTab::ExpandTopLevelItems()
{
    for (u32 iModel = 0; iModel < 2; iModel++)
    {
        QAbstractItemModel *pModel = (iModel == 0 ? &mLayersProxyModel : &mTypesProxyModel);
        QTreeView *pView = (iModel == 0 ? ui->LayersTreeView : ui->TypesTreeView);
        QModelIndex Index = pModel->index(0,0);

        while (Index.isValid())
        {
            pView->expand(Index);
            Index = Index.sibling(Index.row() + 1, 0);
        }
    }
}
