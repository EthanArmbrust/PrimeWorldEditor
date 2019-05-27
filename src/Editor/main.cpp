#include "CEditorApplication.h"
#include "CUIRelay.h"
#include "UICommon.h"
#include <Common/Log.h>

#include <Core/NCoreTests.h>
#include <Core/Resource/Script/NGameList.h>

#include <QApplication>
#include <QIcon>
#include <QStyleFactory>
#include <QtGlobal>

#ifdef __APPLE__
void MacOSSetDarkAppearance();
#endif

// Redirect qDebug output to the log file
void QtLogRedirect(QtMsgType Type, const QMessageLogContext& /*rkContext*/, const QString& rkMessage)
{
    switch (Type)
    {
    case QtDebugMsg:    debugf("Qt Debug: %s",    *TO_TSTRING(rkMessage)); break;
    case QtWarningMsg:  warnf ("Qt Warning: %s",  *TO_TSTRING(rkMessage)); break;
    case QtCriticalMsg: errorf("Qt Critical: %s", *TO_TSTRING(rkMessage)); break;
    case QtFatalMsg:    fatalf("Qt Fatal: %s",    *TO_TSTRING(rkMessage)); break;
    default: break;
    }
}

static TString LocateDataDirectory()
{
#if !defined(_WIN32) && !defined(__APPLE__)
#ifdef PWE_DATADIR
    {
        /* This is for build-configured root */
        TString dir = FileUtil::MakeAbsolute(PWE_DATADIR);
        debugf("Checking '%s' for resources", *dir);
        if (FileUtil::IsDirectory(dir + "resources"))
            return dir;
    }
#endif
    {
        /* This is for locating appimage root */
        TString dir = FileUtil::MakeAbsolute(TString(QCoreApplication::applicationDirPath().toUtf8().data()) + "/../share/PrimeWorldEditor");
        debugf("Checking '%s' for resources", *dir);
        if (FileUtil::IsDirectory(dir + "resources"))
            return dir;
    }
#endif
    {
        /* This is for locating build directory root */
        TString dir = FileUtil::MakeAbsolute(TString(QCoreApplication::applicationDirPath().toUtf8().data()) + "/..");
        debugf("Checking '%s' for resources", *dir);
        if (FileUtil::IsDirectory(dir + "resources"))
            return dir;
    }
    TString dir = FileUtil::MakeAbsolute("..");
    warnf("Falling back to '%s' for resources", *dir);
    return dir;
}

static TString LocateLogPath()
{
#ifndef _WIN32
    return TString(getenv("HOME")) + "/.primeworldeditor.log";
#else
    return "primeworldeditor.log";
#endif
}

class CMain
{
public:
    /** Main function */
    int Main(int argc, char *argv[])
    {
        // Create application
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
        QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
        QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
        CEditorApplication App(argc, argv);
        App.setApplicationName( APP_NAME );
        App.setApplicationVersion( APP_VERSION );
        App.setOrganizationName("Aruki");
        App.setWindowIcon(QIcon(":/icons/win/AppIcon.ico"));

        // Create UI relay
        CUIRelay UIRelay(&App);
        gpUIRelay = &UIRelay;

        // Set up dark theme
        qApp->setStyle(QStyleFactory::create("Fusion"));
        SetupPalette();
#ifdef __APPLE__
        MacOSSetDarkAppearance();
#endif

        // Init log
        bool Initialized = NLog::InitLog(LocateLogPath());
        if (!Initialized) UICommon::ErrorMsg(0, "Couldn't open log file. Logging will not work for this session.");
        qInstallMessageHandler(QtLogRedirect);

        // Locate data directory
        gDataDir = LocateDataDirectory();

        // Create editor resource store
        gpEditorStore = new CResourceStore(gDataDir + "resources/");

        if (!gpEditorStore->DatabasePathExists())
        {
            UICommon::ErrorMsg(0, "Unable to locate PWE resources directory; "
                                  "PWE's executable must remain as deployed.");
            return 1;
        }

        if (!gpEditorStore->AreAllEntriesValid())
        {
            debugf("Editor store has invalid entries. Rebuilding database...");
            gpEditorStore->RebuildFromDirectory();
            gpEditorStore->ConditionalSaveStore();
        }

        // Check for unit tests being run
        if ( NCoreTests::RunTests(argc, argv) )
        {
            return 0;
        }

        // Execute application
        App.InitEditor();
        return App.exec();
    }

    /** Clean up any resources at the end of application execution */
    ~CMain()
    {
        NGameList::Shutdown();
    }

    void SetupPalette()
    {
        QPalette DarkPalette;
        ConfigureColorGroup(DarkPalette, QPalette::Active, 100);
        ConfigureColorGroup(DarkPalette, QPalette::Inactive, 100);
        ConfigureColorGroup(DarkPalette, QPalette::Disabled, 200);
        qApp->setPalette(DarkPalette);
    }

    void ConfigureColorGroup(QPalette& Palette, QPalette::ColorGroup Group, int Factor)
    {
        Palette.setColor( Group, QPalette::Window,           QColor(53,53,53)  .darker(Factor) );
        Palette.setColor( Group, QPalette::WindowText,       QColor(Qt::white) .darker(Factor) );
        Palette.setColor( Group, QPalette::Base,             QColor(25,25,25)  .darker(Factor) );
        Palette.setColor( Group, QPalette::AlternateBase,    QColor(35,35,35)  .darker(Factor) );
        Palette.setColor( Group, QPalette::ToolTipBase,      QColor(Qt::white) .darker(Factor) );
        Palette.setColor( Group, QPalette::ToolTipText,      QColor(Qt::white) .darker(Factor) );
        Palette.setColor( Group, QPalette::Text,             QColor(Qt::white) .darker(Factor) );
        Palette.setColor( Group, QPalette::Button,           QColor(53,53,53)  .darker(Factor) );
        Palette.setColor( Group, QPalette::ButtonText,       QColor(Qt::white) .darker(Factor) );
        Palette.setColor( Group, QPalette::BrightText,       QColor(Qt::red)   .darker(Factor) );
        Palette.setColor( Group, QPalette::Link,             QColor(42,130,218).darker(Factor) );
        Palette.setColor( Group, QPalette::Highlight,        QColor(42,130,218).darker(Factor) );
        Palette.setColor( Group, QPalette::HighlightedText,  QColor(Qt::white) .darker(Factor) );
    }
};

int main(int argc, char *argv[])
{
    CMain Main;
    return Main.Main(argc, argv);
}
