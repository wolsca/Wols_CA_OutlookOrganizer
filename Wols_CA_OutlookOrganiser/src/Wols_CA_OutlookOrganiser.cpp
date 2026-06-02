#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QTextStream>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "ConfigManager.hpp"
#include "MainWindow.hpp"
#include "OutlookManager.hpp"
#include "RulesManager.hpp"

static QString g_logFilePath;

void WolsQtLogHandler( QtMsgType type, const QMessageLogContext &context, const QString &msg )
{
    QString txt;
    switch ( type )
    {
        case QtDebugMsg:
            txt = QString( "[DEBUG] %1" ).arg( msg );
            break;
        case QtInfoMsg:
            txt = QString( "[INFO] %1" ).arg( msg );
            break;
        case QtWarningMsg:
            txt = QString( "[WARNING] %1" ).arg( msg );
            break;
        case QtCriticalMsg:
            txt = QString( "[CRITICAL] %1" ).arg( msg );
            break;
        case QtFatalMsg:
            txt = QString( "[FATAL] %1" ).arg( msg );
            break;
    }
    QString timestamp = QDateTime::currentDateTime().toString( "yyyy-MM-dd HH:mm:ss" );
    QString formatted = QString( "[%1] %2" ).arg( timestamp, txt );

    if ( !g_logFilePath.isEmpty() )
    {
        QFile file( g_logFilePath );
        if ( file.open( QIODevice::Append | QIODevice::Text ) )
        {
            QTextStream out( &file );
            out << formatted << "\n";
            file.close();
        }
    }
    OutputDebugStringW( reinterpret_cast<const wchar_t *>( formatted.utf16() ) );
    OutputDebugStringW( L"\n" );
}

int main( int argc, char *argv[] )
{
    QString baseDir = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/OutlookManager";
    QDir().mkpath( baseDir + "/Logs" );
    g_logFilePath = baseDir + "/Logs/Wols_CA_OutlookOrganiser.log";

    qInstallMessageHandler( WolsQtLogHandler );

    qInfo() << "===========================================================";
    qInfo() << "Wols CA Outlook Organiser - Boot Sequence Started";
    qInfo() << "===========================================================";

    QApplication::setHighDpiScaleFactorRoundingPolicy( Qt::HighDpiScaleFactorRoundingPolicy::PassThrough );
    QApplication app( argc, argv );
    app.setApplicationName( "Wols CA Outlook Organiser" );
    app.setOrganizationName( "Wols CA" );

    QApplication::setQuitOnLastWindowClosed( false );

    if ( !QSystemTrayIcon::isSystemTrayAvailable() )
    {
        QMessageBox::critical( nullptr, "Systeemfout", "Systeemvak (Tray) is niet beschikbaar." );
        return 1;
    }

    ConfigManager  config;
    RulesManager   rulesManager( baseDir );
    OutlookManager outlook( &config, &rulesManager );

    // OPMERKING: Geen outlook.InitializeCOM() meer nodig!
    // De manager opent nu zelf dynamische sessies.

    MainWindow window( &config, &rulesManager, &outlook );

    bool autoStart = false;
    for ( int i = 1; i < argc; ++i )
    {
        if ( QString( argv[i] ) == "--autostart" )
        {
            autoStart = true;
            break;
        }
    }

    if ( autoStart )
    {
        qInfo() << "Gestart via autostart (Tray).";
    }
    else
    {
        qInfo() << "Handmatig gestart. Toon hoofdvenster.";
        window.show();
    }

    return app.exec();
}