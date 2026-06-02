#pragma once

#include <QJsonObject>
#include <QString>

class ConfigManager
{
  public:
    ConfigManager();
    ConfigManager( const QString &customPath );
    ~ConfigManager();

    // --------------------------------------------------------------------------
    // Core Configuration Matrix Accessors
    // --------------------------------------------------------------------------
    QString GetConfigString( const QString &key );
    void    SetConfigString( const QString &key, const QString &value );
    int     GetConfigInt( const QString &key, int defaultVal = 0 );
    QString GetText( const QString &key );

    // --------------------------------------------------------------------------
    // Global Registry / OS Settings (via QSettings)
    // --------------------------------------------------------------------------
    bool GetRegistryBool( const QString &keyName, bool defaultValue );
    void SetRegistryBool( const QString &keyName, bool value );

    // --------------------------------------------------------------------------
    // Secure Dual-Layer Folder Mapping Sub-System
    // --------------------------------------------------------------------------
    bool LoadUserFolders( QJsonObject &outUserJson );
    bool SaveUserFolders( const QJsonObject &userJson );
    bool LoadSystemFolders( QJsonObject &outSystemJson );
    bool SaveSystemConfiguration( const QJsonObject &systemJson );

    // --------------------------------------------------------------------------
    // Disaster Recovery Points
    // --------------------------------------------------------------------------
    bool BackupFolderMapping();
    bool RestoreFolderMapping();

    // --------------------------------------------------------------------------
    // Operating System Boundary Path Resolvers
    // --------------------------------------------------------------------------
    QString GetAccountDirectoryPath();

  private:
    void LoadMainConfig();
    void SaveMainConfig();

    void UnlockFileForWriting( const QString &filePath );
    void LockAndHideFile( const QString &filePath );

    QString     m_mainConfigPath;
    QJsonObject m_mainConfig;
};
