/*
 * MIT License
 * Copyright (c) 2026 Wols CA / Oskar Wols
 */

#include "Logger.hpp"
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>
#include <iostream>

// Initialize static members
QFile *Logger::s_logFile = nullptr;
QMutex Logger::s_mutex;

void Logger::Initialize()
{
    // Define the secure and standard Wols CA log path
    QString logDir = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/Wols_CA_OutlookOrganiser/Logs";
    QDir().mkpath( logDir );

    QString logFilePath = logDir + "/Wols_CA_Engine.log";

    s_logFile = new QFile( logFilePath );

    // Open the file in Append and Text mode
    if ( s_logFile->open( QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text ) )
    {
        // Hook into the Qt messaging system
        qInstallMessageHandler( Logger::MessageHandler );

        qInfo() << "==============================================================================";
        qInfo() << "[SYSTEM] Wols CA Outlook Organiser Engine Initialized";
    }
    else
    {
        qWarning( "Failed to open log file at %s", qPrintable( logFilePath ) );
    }
}

void Logger::Shutdown()
{
    if ( s_logFile )
    {
        qInfo() << "[SYSTEM] Wols CA Outlook Organiser Engine Shutdown";
        qInfo() << "==============================================================================";

        // Remove the hook and safely close the file
        qInstallMessageHandler( nullptr );

        s_logFile->close();
        delete s_logFile;
        s_logFile = nullptr;
    }
}

void Logger::MessageHandler( QtMsgType type, const QMessageLogContext &context, const QString &msg )
{
    // Lock the mutex to ensure thread-safe writing from AsyncWorkers
    QMutexLocker locker( &s_mutex );

    QString timeStr = QDateTime::currentDateTime().toString( "yyyy-MM-dd HH:mm:ss.zzz" );
    QString levelStr;

    switch ( type )
    {
        case QtDebugMsg:
            levelStr = "[DEBUG] ";
            break;
        case QtInfoMsg:
            levelStr = "[INFO ] ";
            break;
        case QtWarningMsg:
            levelStr = "[WARN ] ";
            break;
        case QtCriticalMsg:
            levelStr = "[CRIT ] ";
            break;
        case QtFatalMsg:
            levelStr = "[FATAL] ";
            break;
    }

    // Format the message
    QString formattedMessage = QString( "%1 %2 %3\n" ).arg( timeStr, levelStr, msg );

    // 1. Write to the file
    if ( s_logFile && s_logFile->isOpen() )
    {
        QTextStream out( s_logFile );
        out << formattedMessage;
        out.flush();
    }

    // 2. Also output to the Visual Studio / Windows console for real-time debugging
    QTextStream console( stdout );
    console << formattedMessage;
    console.flush();
}