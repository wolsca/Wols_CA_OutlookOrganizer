#include "RulesManager.hpp"
#include "RulesEngine.hpp"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>

RulesManager::RulesManager( const QString &baseAppDir ) : m_baseAppDir( baseAppDir )
{
}

RulesManager::~RulesManager()
{
    SaveAllRules();
}

QString RulesManager::SanitizeAccountName( const QString &email )
{
    QString safeName = email;
    safeName.replace( "@", "_" );
    safeName.replace( ".", "_" );
    return safeName;
}

QString RulesManager::GetTimestamp()
{
    return QDateTime::currentDateTime().toString( "yyyyMMdd_HHmmss" );
}

uint64_t RulesManager::GetNextId()
{
    m_globalIdCounter++;
    return m_globalIdCounter;
}

void RulesManager::LoadForAccount( const QString &accountName )
{
    QString safeAccount = SanitizeAccountName( accountName );

    // Volledig platform-onafhankelijke documenten map via Qt
    QString docsPath = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation );
    QDir    targetDir( docsPath + "/OutlookManager/Accounts/" + safeAccount );

    if ( !targetDir.exists() )
    {
        targetDir.mkpath( "." );
    }

    m_rulesFilePath = targetDir.absoluteFilePath( "routing_rules_v3.json" );
    m_backupDirPath = targetDir.absoluteFilePath( "Backups" );

    QDir backupDir( m_backupDirPath );
    if ( !backupDir.exists() )
    {
        backupDir.mkpath( "." );
    }

    LoadRules();
}

void RulesManager::LoadRules()
{
    m_systemFolders.clear();
    m_organizations.clear();
    m_contacts.clear();
    m_tenders.clear();

    QFile file( m_rulesFilePath );
    if ( file.exists() )
    {
        if ( file.open( QIODevice::ReadOnly ) )
        {
            QJsonDocument doc = QJsonDocument::fromJson( file.readAll() );
            if ( !doc.isNull() && doc.isObject() )
            {
                m_rawJson = doc.object();
            }
            file.close();
        }
    }
    else
    {
        QFileInfo fileInfo( m_rulesFilePath );
        QFile     legacyFile( fileInfo.dir().absoluteFilePath( "routing_rules.json" ) );
        if ( legacyFile.exists() )
        {
            RunMigrationToV3();
            return;
        }
    }

    if ( m_rawJson.contains( "global_id_counter" ) )
    {
        m_globalIdCounter = m_rawJson["global_id_counter"].toVariant().toULongLong();
    }

    if ( m_rawJson.contains( "system_folders" ) )
    {
        QJsonObject sysObj = m_rawJson["system_folders"].toObject();
        for ( auto it = sysObj.begin(); it != sysObj.end(); ++it )
        {
            m_systemFolders[it.key()] = it.value().toObject().value( "folder_id" ).toString();
        }
    }

    if ( m_rawJson.contains( "organizations" ) && m_rawJson["organizations"].isArray() )
    {
        QJsonArray orgsArray = m_rawJson["organizations"].toArray();
        for ( const QJsonValue &orgVal : orgsArray )
        {
            QJsonObject      orgJson = orgVal.toObject();
            OrganizationRule org;
            org.id          = orgJson.value( "id" ).toVariant().toULongLong();
            org.name        = orgJson.value( "name" ).toString();
            org.longName    = orgJson.value( "long_name" ).toString();
            org.description = orgJson.value( "description" ).toString();
            org.folderId    = orgJson.value( "folder_id" ).toString();

            if ( orgJson.contains( "domain_catchall" ) )
            {
                for ( const auto &dom : orgJson["domain_catchall"].toArray() )
                {
                    org.domainCatchall.append( dom.toString() );
                }
            }

            if ( orgJson.contains( "topics" ) )
            {
                for ( const auto &topicVal : orgJson["topics"].toArray() )
                {
                    QJsonObject topicJson = topicVal.toObject();
                    TopicRule   topic;
                    topic.id            = topicJson.value( "id" ).toVariant().toULongLong();
                    topic.name          = topicJson.value( "name" ).toString();
                    topic.longName      = topicJson.value( "long_name" ).toString();
                    topic.description   = topicJson.value( "description" ).toString();
                    topic.folderId      = topicJson.value( "folder_id" ).toString();
                    topic.minMatchCount = topicJson.value( "min_match_count" ).toInt( 1 );

                    for ( const auto &kw : topicJson["keywords"].toArray() )
                    {
                        topic.keywords.append( kw.toString() );
                    }
                    org.topics.append( topic );
                }
            }

            if ( orgJson.contains( "contacts" ) )
            {
                for ( const auto &cntVal : orgJson["contacts"].toArray() )
                {
                    QJsonObject    cntJson = cntVal.toObject();
                    OrgContactRule contact;
                    contact.id          = cntJson.value( "id" ).toVariant().toULongLong();
                    contact.name        = cntJson.value( "name" ).toString();
                    contact.longName    = cntJson.value( "long_name" ).toString();
                    contact.description = cntJson.value( "description" ).toString();
                    contact.email       = cntJson.value( "email" ).toString();
                    contact.folderId    = cntJson.value( "folder_id" ).toString();
                    org.contacts.append( contact );
                }
            }
            m_organizations.append( org );
        }
    }

    if ( m_rawJson.contains( "contacts" ) && m_rawJson["contacts"].isArray() )
    {
        for ( const QJsonValue &cntVal : m_rawJson["contacts"].toArray() )
        {
            QJsonObject cntJson = cntVal.toObject();
            ContactRule contact;
            contact.id          = cntJson.value( "id" ).toVariant().toULongLong();
            contact.name        = cntJson.value( "name" ).toString();
            contact.longName    = cntJson.value( "long_name" ).toString();
            contact.description = cntJson.value( "description" ).toString();
            contact.folderId    = cntJson.value( "folder_id" ).toString();

            for ( const auto &em : cntJson["emails"].toArray() )
            {
                contact.emails.append( em.toString() );
            }
            m_contacts.append( contact );
        }
    }

    if ( m_rawJson.contains( "tenders" ) && m_rawJson["tenders"].isArray() )
    {
        for ( const QJsonValue &tndVal : m_rawJson["tenders"].toArray() )
        {
            QJsonObject tndJson = tndVal.toObject();
            TenderRule  tender;
            tender.id          = tndJson.value( "id" ).toVariant().toULongLong();
            tender.name        = tndJson.value( "name" ).toString();
            tender.longName    = tndJson.value( "long_name" ).toString();
            tender.description = tndJson.value( "description" ).toString();
            tender.folderId    = tndJson.value( "folder_id" ).toString();

            for ( const auto &kw : tndJson["keywords"].toArray() )
            {
                tender.keywords.append( kw.toString() );
            }

            if ( tndJson.contains( "phases" ) )
            {
                for ( const auto &phsVal : tndJson["phases"].toArray() )
                {
                    QJsonObject phsJson = phsVal.toObject();
                    TenderPhase phase;
                    phase.id          = phsJson.value( "id" ).toVariant().toULongLong();
                    phase.name        = phsJson.value( "name" ).toString();
                    phase.longName    = phsJson.value( "long_name" ).toString();
                    phase.description = phsJson.value( "description" ).toString();
                    phase.folderId    = phsJson.value( "folder_id" ).toString();
                    tender.phases.append( phase );
                }
            }
            m_tenders.append( tender );
        }
    }
}

void RulesManager::RunMigrationToV3()
{
    m_rawJson["schema_version"]    = "3.0";
    m_rawJson["global_id_counter"] = ( qint64 )m_globalIdCounter;
    SaveAllRules();
}

RoutingDecision RulesManager::DetermineRoute( const QString &senderEmail, const QString &subject, const QString &body )
{
    return RulesEngine::DetermineRoute( m_organizations, m_contacts, m_tenders, senderEmail, subject, body );
}

void RulesManager::SaveAllRules()
{
    if ( m_rulesFilePath.isEmpty() )
    {
        return;
    }

    m_rawJson["global_id_counter"] = ( qint64 )m_globalIdCounter;
    m_rawJson["schema_version"]    = "3.0";

    QJsonArray orgsArray;
    for ( const auto &org : m_organizations )
    {
        QJsonObject orgJson;
        orgJson["id"]          = ( qint64 )org.id;
        orgJson["name"]        = org.name;
        orgJson["long_name"]   = org.longName;
        orgJson["description"] = org.description;
        orgJson["folder_id"]   = org.folderId;

        QJsonArray domArray;
        for ( const auto &d : org.domainCatchall )
        {
            domArray.append( d );
        }
        orgJson["domain_catchall"] = domArray;

        QJsonArray topicsArray;
        for ( const auto &topic : org.topics )
        {
            QJsonObject topicJson;
            topicJson["id"]              = ( qint64 )topic.id;
            topicJson["name"]            = topic.name;
            topicJson["long_name"]       = topic.longName;
            topicJson["description"]     = topic.description;
            topicJson["folder_id"]       = topic.folderId;
            topicJson["min_match_count"] = topic.minMatchCount;

            QJsonArray kwArray;
            for ( const auto &k : topic.keywords )
            {
                kwArray.append( k );
            }
            topicJson["keywords"] = kwArray;
            topicsArray.append( topicJson );
        }
        orgJson["topics"] = topicsArray;

        QJsonArray contactsArray;
        for ( const auto &contact : org.contacts )
        {
            QJsonObject cntJson;
            cntJson["id"]          = ( qint64 )contact.id;
            cntJson["name"]        = contact.name;
            cntJson["long_name"]   = contact.longName;
            cntJson["description"] = contact.description;
            cntJson["email"]       = contact.email;
            cntJson["folder_id"]   = contact.folderId;
            contactsArray.append( cntJson );
        }
        orgJson["contacts"] = contactsArray;

        orgsArray.append( orgJson );
    }
    m_rawJson["organizations"] = orgsArray;

    QJsonArray cntsArray;
    for ( const auto &contact : m_contacts )
    {
        QJsonObject cntJson;
        cntJson["id"]          = ( qint64 )contact.id;
        cntJson["name"]        = contact.name;
        cntJson["long_name"]   = contact.longName;
        cntJson["description"] = contact.description;
        cntJson["folder_id"]   = contact.folderId;

        QJsonArray emArray;
        for ( const auto &e : contact.emails )
        {
            emArray.append( e );
        }
        cntJson["emails"] = emArray;
        cntsArray.append( cntJson );
    }
    m_rawJson["contacts"] = cntsArray;

    QJsonArray tendersArray;
    for ( const auto &tender : m_tenders )
    {
        QJsonObject tndJson;
        tndJson["id"]          = ( qint64 )tender.id;
        tndJson["name"]        = tender.name;
        tndJson["long_name"]   = tender.longName;
        tndJson["description"] = tender.description;
        tndJson["folder_id"]   = tender.folderId;

        QJsonArray kwArray;
        for ( const auto &k : tender.keywords )
        {
            kwArray.append( k );
        }
        tndJson["keywords"] = kwArray;

        QJsonArray phasesArray;
        for ( const auto &phase : tender.phases )
        {
            QJsonObject phsJson;
            phsJson["id"]          = ( qint64 )phase.id;
            phsJson["name"]        = phase.name;
            phsJson["long_name"]   = phase.longName;
            phsJson["description"] = phase.description;
            phsJson["folder_id"]   = phase.folderId;
            phasesArray.append( phsJson );
        }
        tndJson["phases"] = phasesArray;

        tendersArray.append( tndJson );
    }
    m_rawJson["tenders"] = tendersArray;

    QFile file( m_rulesFilePath );
    if ( file.open( QIODevice::WriteOnly ) )
    {
        QJsonDocument doc( m_rawJson );
        file.write( doc.toJson( QJsonDocument::Indented ) );
        file.close();
    }
}

void RulesManager::ClearAllRules()
{
    m_rawJson         = QJsonObject();
    m_globalIdCounter = 0;
    m_organizations.clear();
    m_contacts.clear();
    m_tenders.clear();
    SaveAllRules();
}

QString RulesManager::GetRawRulesJSON()
{
    QJsonDocument doc( m_rawJson );
    return QString::fromUtf8( doc.toJson( QJsonDocument::Indented ) );
}

bool RulesManager::SetRawRulesJSON( const QString &jsonStr )
{
    QJsonParseError error;
    QJsonDocument   doc = QJsonDocument::fromJson( jsonStr.toUtf8(), &error );
    if ( error.error == QJsonParseError::NoError && doc.isObject() )
    {
        m_rawJson = doc.object();
        SaveAllRules();
        LoadRules();
        return true;
    }
    return false;
}

QList<QString> RulesManager::GetAllKnownOrganizations()
{
    QList<QString> result;
    for ( const auto &org : m_organizations )
    {
        result.append( org.name );
    }
    return result;
}

bool RulesManager::CreateBackup()
{
    if ( m_backupDirPath.isEmpty() )
    {
        return false;
    }

    QString timestamp = GetTimestamp();
    QFile   dest( QDir( m_backupDirPath ).absoluteFilePath( "rules_backup_" + timestamp + ".json" ) );

    QJsonObject backupData;
    backupData["timestamp"] = timestamp;
    backupData["data"]      = m_rawJson;

    if ( dest.open( QIODevice::WriteOnly ) )
    {
        QJsonDocument doc( backupData );
        dest.write( doc.toJson( QJsonDocument::Indented ) );
        dest.close();
        return true;
    }
    return false;
}

QList<QString> RulesManager::ListBackups()
{
    QList<QString> backups;
    if ( m_backupDirPath.isEmpty() )
    {
        return backups;
    }

    QDir dir( m_backupDirPath );
    for ( const QFileInfo &info : dir.entryInfoList( { "*.json" }, QDir::Files, QDir::Time ) )
    {
        backups.append( info.fileName() );
    }
    return backups;
}

bool RulesManager::RestoreBackup( const QString &filename )
{
    if ( m_backupDirPath.isEmpty() )
    {
        return false;
    }

    QFile file( QDir( m_backupDirPath ).absoluteFilePath( filename ) );
    if ( !file.open( QIODevice::ReadOnly ) )
    {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson( file.readAll() );
    file.close();

    if ( !doc.isNull() && doc.isObject() && doc.object().contains( "data" ) )
    {
        m_rawJson = doc.object()["data"].toObject();
        SaveAllRules();
        LoadRules();
        return true;
    }
    return false;
}

void RulesManager::UpdateSystemFolderId( const QString &systemKey, const QString &entryId )
{
    m_systemFolders[systemKey] = entryId;
    if ( m_rawJson.contains( "system_folders" ) )
    {
        QJsonObject sysObj          = m_rawJson["system_folders"].toObject();
        QJsonObject targetObj       = sysObj.value( systemKey ).toObject();
        targetObj["folder_id"]      = entryId;
        sysObj[systemKey]           = targetObj;
        m_rawJson["system_folders"] = sysObj;
    }
}

void RulesManager::UpdateRuleFolderId( uint64_t ruleId, const QString &type, const QString &entryId )
{
    for ( auto &org : m_organizations )
    {
        if ( org.id == ruleId )
        {
            org.folderId = entryId;
            SaveAllRules();
            return;
        }
        for ( auto &topic : org.topics )
        {
            if ( topic.id == ruleId )
            {
                topic.folderId = entryId;
                SaveAllRules();
                return;
            }
        }
        for ( auto &contact : org.contacts )
        {
            if ( contact.id == ruleId )
            {
                contact.folderId = entryId;
                SaveAllRules();
                return;
            }
        }
    }
    for ( auto &contact : m_contacts )
    {
        if ( contact.id == ruleId )
        {
            contact.folderId = entryId;
            SaveAllRules();
            return;
        }
    }
    for ( auto &tender : m_tenders )
    {
        if ( tender.id == ruleId )
        {
            tender.folderId = entryId;
            SaveAllRules();
            return;
        }
        for ( auto &phase : tender.phases )
        {
            if ( phase.id == ruleId )
            {
                phase.folderId = entryId;
                SaveAllRules();
                return;
            }
        }
    }
}

void RulesManager::SaveOrganization( const OrganizationRule &org )
{
    bool found = false;
    for ( auto &existing : m_organizations )
    {
        if ( existing.id == org.id )
        {
            existing = org;
            found    = true;
            break;
        }
    }
    if ( !found )
    {
        OrganizationRule newOrg = org;
        if ( newOrg.id == 0 )
        {
            newOrg.id = GetNextId();
        }
        m_organizations.append( newOrg );
    }
    SaveAllRules();
}

void RulesManager::SaveContact( const ContactRule &contact )
{
    bool found = false;
    for ( auto &existing : m_contacts )
    {
        if ( existing.id == contact.id )
        {
            existing = contact;
            found    = true;
            break;
        }
    }
    if ( !found )
    {
        ContactRule newContact = contact;
        if ( newContact.id == 0 )
        {
            newContact.id = GetNextId();
        }
        m_contacts.append( newContact );
    }
    SaveAllRules();
}

void RulesManager::SaveTender( const TenderRule &tender )
{
    bool found = false;
    for ( auto &existing : m_tenders )
    {
        if ( existing.id == tender.id )
        {
            existing = tender;
            found    = true;
            break;
        }
    }
    if ( !found )
    {
        TenderRule newTender = tender;
        if ( newTender.id == 0 )
        {
            newTender.id = GetNextId();
        }
        m_tenders.append( newTender );
    }
    SaveAllRules();
}
