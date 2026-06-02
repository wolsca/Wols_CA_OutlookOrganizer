/*
 * MIT License
 * Copyright (c) 2026 Wols CA / Oskar Wols
 */

#pragma once

#include <QFile>
#include <QMessageLogContext>
#include <QMutex>
#include <QString>

class Logger
{
  public:
    // Call this once in main.cpp before starting the QApplication
    static void Initialize();

    // Call this before exiting main.cpp to safely flush and close the file
    static void Shutdown();

  private:
    // The central callback that intercepts all qDebug, qInfo, qWarning, etc.
    static void MessageHandler( QtMsgType type, const QMessageLogContext &context, const QString &msg );

    static QFile *s_logFile;
    static QMutex s_mutex; // Ensures thread-safety for background tasks
};