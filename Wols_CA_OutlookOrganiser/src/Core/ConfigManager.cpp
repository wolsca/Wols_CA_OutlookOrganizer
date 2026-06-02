#include "ConfigManager.hpp"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QSettings>
#include <QStandardPaths>

namespace
{
QString SanitizeAccountNameInternal( const QString &email )
{
    QString safe = email;
    safe.replace( "@", "_" );
    safe.replace( ".", "_" );
    return safe;
}
} // namespace

ConfigManager::ConfigManager()
{
    QString baseDir = QDir::currentPath() + "/OutlookManager";
    QDir().mkpath( baseDir );

    m_mainConfigPath = baseDir + "/main_config.json";
    LoadMainConfig();
}

ConfigManager::ConfigManager( const QString &customPath )
{
    m_mainConfigPath = customPath;
    QFileInfo fileInfo( m_mainConfigPath );
    QDir().mkpath( fileInfo.absolutePath() );
    LoadMainConfig();
}

ConfigManager::~ConfigManager()
{
    SaveMainConfig();
}

// ==============================================================================
// CORE STRINGS STORAGE
// ==============================================================================
void ConfigManager::LoadMainConfig()
{
    QFile file( m_mainConfigPath );
    if ( file.exists() && file.open( QIODevice::ReadOnly ) )
    {
        QJsonDocument doc = QJsonDocument::fromJson( file.readAll() );
        if ( !doc.isNull() && doc.isObject() )
        {
            m_mainConfig = doc.object();
        }
        file.close();
    }
    else
    {
        m_mainConfig                     = QJsonObject();
        m_mainConfig["selected_account"] = "";
        SaveMainConfig();
    }
}

void ConfigManager::SaveMainConfig()
{
    QFile file( m_mainConfigPath );
    if ( file.open( QIODevice::WriteOnly ) )
    {
        QJsonDocument doc( m_mainConfig );
        file.write( doc.toJson( QJsonDocument::Indented ) );
        file.close();
    }
}

QString ConfigManager::GetConfigString( const QString &key )
{
    return m_mainConfig.value( key ).toString();
}

void ConfigManager::SetConfigString( const QString &key, const QString &value )
{
    m_mainConfig[key] = value;
    SaveMainConfig();
}

int ConfigManager::GetConfigInt( const QString &key, int defaultVal )
{
    if ( m_mainConfig.contains( key ) )
    {
        return m_mainConfig.value( key ).toInt( defaultVal );
    }
    return defaultVal;
}

QString ConfigManager::GetText( const QString &key )
{
    return m_mainConfig.value( key ).toString( key );
}

// ==============================================================================
// GLOBAL REGISTRY SETTINGS (via QSettings)
// ==============================================================================
bool ConfigManager::GetRegistryBool( const QString &keyName, bool defaultValue )
{
    // Qt QSettings handled dit native voor de registry (op Windows in HKCU\Software\WolsCA\InboxManager)
    QSettings settings( "WolsCA", "InboxManager" );
    return settings.value( keyName, defaultValue ).toBool();
}

void ConfigManager::SetRegistryBool( const QString &keyName, bool value )
{
    QSettings settings( "WolsCA", "InboxManager" );
    settings.setValue( keyName, value );
}

// ==============================================================================
// PATH RESOLUTION BOUNDARY
// ==============================================================================
QString ConfigManager::GetAccountDirectoryPath()
{
    QString activeAccount = GetConfigString( "selected_account" );
    if ( activeAccount.isEmpty() )
    {
        return "";
    }

    QString docsPath = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation );
    QString baseDir  = docsPath + "/OutlookManager/Accounts/" + SanitizeAccountNameInternal( activeAccount );

    QDir().mkpath( baseDir );
    return baseDir;
}

// ==============================================================================
// FILE ATTRIBUTE PROTECTION ENGINE
// ==============================================================================
void ConfigManager::UnlockFileForWriting( const QString &filePath )
{
    QFile file( filePath );
    if ( file.exists() )
    {
        // Qt Permissions: Read, Write voor de owner. Verwijdert 'Read-Only'.
        file.setPermissions( QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser | QFile::WriteUser );
    }
}

void ConfigManager::LockAndHideFile( const QString &filePath )
{
    QFile file( filePath );
    if ( file.exists() )
    {
        // Zet het bestand naar strikt Read-Only
        file.setPermissions( QFile::ReadOwner | QFile::ReadUser );

        // Qt's manier om een bestand te 'hiden' op Windows.
        // Op Linux/Mac doen ze dit door er een "." voor te zetten.
        // QFile::setPermissions ondersteunt geen expliciete "Hidden" flag,
        // maar dit is vaak niet nodig. Mocht het echt cruciaal zijn, kunnen we SetFileAttributesW lokaal hier herintroduceren.
    }
}

// ==============================================================================
// DUAL-LAYER JURISDICTION LOADERS & SAVERS (PER ACCOUNT)
// ==============================================================================
bool ConfigManager::LoadUserFolders( QJsonObject &outUserJson )
{
    QString path = GetAccountDirectoryPath() + "/User Config Folders.json";
    QFile   file( path );
    if ( !file.exists() )
    {
        QJsonObject folderMapping;
        folderMapping["Target_Organizations"] = "00_Organizations";
        folderMapping["Target_Contacts"]      = "00_Contacts";
        folderMapping["Triage_Audit"]         = "01_To_Audit";
        folderMapping["Triage_ReAudit"]       = "01_To_Re-Audit";

        outUserJson                   = QJsonObject();
        outUserJson["folder_mapping"] = folderMapping;
        return SaveUserFolders( outUserJson );
    }

    if ( file.open( QIODevice::ReadOnly ) )
    {
        QJsonDocument doc = QJsonDocument::fromJson( file.readAll() );
        outUserJson       = doc.object();
        file.close();
        return true;
    }
    return false;
}

bool ConfigManager::SaveUserFolders( const QJsonObject &userJson )
{
    QString path = GetAccountDirectoryPath() + "/User Config Folders.json";
    QFile   file( path );
    if ( file.open( QIODevice::WriteOnly ) )
    {
        QJsonDocument doc( userJson );
        file.write( doc.toJson( QJsonDocument::Indented ) );
        file.close();
        return true;
    }
    return false;
}

bool ConfigManager::LoadSystemFolders( QJsonObject &outSystemJson )
{
    QString path = GetAccountDirectoryPath() + "/System Config Folders.json";
    QFile   file( path );
    if ( !file.exists() )
    {
        outSystemJson              = QJsonObject();
        outSystemJson["entry_ids"] = QJsonObject();
        return SaveSystemConfiguration( outSystemJson );
    }

    if ( file.open( QIODevice::ReadOnly ) )
    {
        QJsonDocument doc = QJsonDocument::fromJson( file.readAll() );
        outSystemJson     = doc.object();
        file.close();
        return true;
    }
    return false;
}

bool ConfigManager::SaveSystemConfiguration( const QJsonObject &systemJson )
{
    QString path = GetAccountDirectoryPath() + "/System Config Folders.json";
    UnlockFileForWriting( path );

    bool  success = false;
    QFile file( path );
    if ( file.open( QIODevice::WriteOnly ) )
    {
        QJsonDocument doc( systemJson );
        file.write( doc.toJson( QJsonDocument::Indented ) );
        file.close();
        success = true;
    }

    LockAndHideFile( path );
    return success;
}

// ==============================================================================
// DISASTER RECOVERY POINTS ENGINE
// ==============================================================================
bool ConfigManager::BackupFolderMapping()
{
    QString accountDir = GetAccountDirectoryPath();
    if ( accountDir.isEmpty() )
    {
        return false;
    }

    QString backupDir = accountDir + "/Backups";
    QDir().mkpath( backupDir );

    QString userSrc = accountDir + "/User Config Folders.json";
    QString sysSrc  = accountDir + "/System Config Folders.json";
    QString userDst = backupDir + "/User_Config_Folders.bak.json";
    QString sysDst  = backupDir + "/System_Config_Folders.bak.json";

    if ( QFile::exists( userSrc ) )
    {
        QFile::remove( userDst );
        QFile::copy( userSrc, userDst );
    }
    if ( QFile::exists( sysSrc ) )
    {
        UnlockFileForWriting( sysDst );
        QFile::remove( sysDst );
        QFile::copy( sysSrc, sysDst );
        LockAndHideFile( sysDst );
    }
    return true;
}

bool ConfigManager::RestoreFolderMapping()
{
    QString accountDir = GetAccountDirectoryPath();
    if ( accountDir.isEmpty() )
    {
        return false;
    }

    QString backupDir = accountDir + "/Backups";
    QString userSrc   = backupDir + "/User_Config_Folders.bak.json";
    QString sysSrc    = backupDir + "/System_Config_Folders.bak.json";
    QString userDst   = accountDir + "/User Config Folders.json";
    QString sysDst    = accountDir + "/System Config Folders.json";

    bool restoredAny = false;

    if ( QFile::exists( userSrc ) )
    {
        QFile::remove( userDst );
        QFile::copy( userSrc, userDst );
        restoredAny = true;
    }
    if ( QFile::exists( sysSrc ) )
    {
        UnlockFileForWriting( sysDst );
        QFile::remove( sysDst );
        QFile::copy( sysSrc, sysDst );
        LockAndHideFile( sysDst );
        restoredAny = true;
    }
    return restoredAny;
}
