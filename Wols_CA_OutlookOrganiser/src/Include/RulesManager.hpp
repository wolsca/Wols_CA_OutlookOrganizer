#pragma once

#include "RuleModels.hpp"
#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QString>

// ==============================================================================
// RULES MANAGER CLASS (Native Qt File & JSON I/O)
// ==============================================================================

class RulesManager
{
  public:
    RulesManager( const QString &baseAppDir );
    ~RulesManager();

    void LoadForAccount( const QString &accountName );

    RoutingDecision DetermineRoute( const QString &senderEmail, const QString &subject, const QString &body );

    void UpdateSystemFolderId( const QString &systemKey, const QString &entryId );
    void UpdateRuleFolderId( uint64_t ruleId, const QString &type, const QString &entryId );

    void           SaveOrganization( const OrganizationRule &org );
    void           SaveContact( const ContactRule &contact );
    void           SaveTender( const TenderRule &tender );
    QList<QString> GetAllKnownOrganizations();

    void SaveAllRules();
    void ClearAllRules();

    QString        GetRawRulesJSON();
    bool           SetRawRulesJSON( const QString &jsonStr );
    bool           CreateBackup();
    QList<QString> ListBackups();
    bool           RestoreBackup( const QString &filename );

  private:
    QString SanitizeAccountName( const QString &email );
    QString GetTimestamp();
    void    LoadRules();
    void    RunMigrationToV3();

    uint64_t GetNextId();

    QString m_baseAppDir;
    QString m_rulesFilePath;
    QString m_backupDirPath;

    QJsonObject m_rawJson;

    uint64_t m_globalIdCounter = 0;

    QHash<QString, QString> m_systemFolders;
    QList<OrganizationRule> m_organizations;
    QList<ContactRule>      m_contacts;
    QList<TenderRule>       m_tenders;
};
