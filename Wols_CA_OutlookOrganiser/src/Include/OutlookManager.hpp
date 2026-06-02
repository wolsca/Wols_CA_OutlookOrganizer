#pragma once

#include "ConfigManager.hpp"
#include "RulesManager.hpp"
#include <QAxObject> // <-- De nieuwe Qt COM module
#include <QObject>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Zorg dat de class overerft van QObject voor het interne Qt geheugenbeheer
class OutlookManager : public QObject
{
  Q_OBJECT // Verplicht voor Qt classes!

      public : explicit OutlookManager( ConfigManager *config, RulesManager *rules, QObject *parent = nullptr );
    ~OutlookManager();

    // ==============================================================================
    // CORE & INITIALIZATION
    // ==============================================================================
    bool                     ConnectToOutlook();
    bool                     InitializeCOM();
    void                     SetBusyCallback( std::function<void( bool )> callback );
    std::vector<std::string> GetAvailableAccounts();

    // ==============================================================================
    // PROCESS & TRIAGE
    // ==============================================================================
    void ProcessInbox( bool fromUI, bool skipUnread );
    bool AuditFolders( bool maxTwoYears );
    void PerformDeepAudit();

    // ==============================================================================
    // RECOVERY
    // ==============================================================================
    bool HasMisplacedContacts( int &outCount );
    void RecoverMisplacedContacts();

  private:
    // ==============================================================================
    // ROUTING & MOVEMENT (Voorheen in routing.hpp gedacht)
    // ==============================================================================
    bool RouteMailItem( QAxObject *mailItem, QAxObject *rootFolder, const std::string &activeAccount, bool isSentItem, bool isAlreadyInQuarantine, bool fromUI, std::unordered_map<std::string, std::unordered_set<std::string>> &sessionTargetCache );
    bool SafeMove( QAxObject *item, QAxObject *targetFolder, const std::string &targetName, const std::string &subj, double timeVal, std::unordered_map<std::string, std::unordered_set<std::string>> &sessionTargetCache );

    // ==============================================================================
    // FOLDER & MAIL HELPERS
    // ==============================================================================
    QAxObject *GetFolderByEntryID( QAxObject *app, const QString &entryIdHex );
    QAxObject *FindFolderByBaseName( QAxObject *parentFolder, const std::string &baseName, std::string &outActualName );
    QAxObject *EnsureFolderExists( QAxObject *parentFolder, const QString &folderName );
    QAxObject *GetOrCreateSystemFolder( QAxObject *parentFolder, const std::string &defaultPrefix, const std::string &baseName );

    std::string GetSenderAddress( QAxObject *mailItem );
    std::string GetMailSubject( QAxObject *mailItem );
    void        SetFlagForProcess( QAxObject *mailItem, int daysOffset );
    void        CleanupTriageFolders();

    // ==============================================================================
    // MEMBER VARIABLES
    // ==============================================================================
    ConfigManager *m_config;
    RulesManager  *m_rules;

    // De oude CComPtr<IDispatch> is nu een strakke Qt pointer
    QAxObject *m_outlookApp;

    std::function<void( bool )> m_busyCallback;
};