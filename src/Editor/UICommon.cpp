#include "UICommon.h"
#include <QDesktopServices>
#include <QProcess>

namespace UICommon
{

QWindow* FindWidgetWindowHandle(QWidget *pWidget)
{
    while (pWidget && !pWidget->windowHandle())
        pWidget = pWidget->parentWidget();

    return pWidget ? pWidget->windowHandle() : nullptr;
}

void OpenContainingFolder(const QString& rkPath)
{
#if WIN32
    QStringList Args;
    Args << "/select," << QDir::toNativeSeparators(rkPath);
    QProcess::startDetached("explorer", Args);
#else
    QStringList Args;
    Args << "/select," << QDir::toNativeSeparators(rkPath);
    QProcess::startDetached("dolphin", Args); //only works with dolphin-fm right now
#endif
}

bool OpenInExternalApplication(const QString& rkPath)
{
    return QDesktopServices::openUrl( QString("file:///") + QDir::toNativeSeparators(rkPath) );
}

}
