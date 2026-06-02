#include "OutlookManager.hpp"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace
{
struct ComThreadScope
{
    CComPtr<IDispatch> app;
    bool               comInit = false;
    ComThreadScope()
    {
        HRESULT hr = CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED );
        comInit    = SUCCEEDED( hr ) || hr == RPC_E_CHANGED_MODE;
        CLSID clsid;
        if ( SUCCEEDED( CLSIDFromProgID( L"Outlook.Application", &clsid ) ) )
        {
            IUnknown *pUnk = nullptr;
            if ( SUCCEEDED( GetActiveObject( clsid, nullptr, &pUnk ) ) && pUnk )
            {
                pUnk->QueryInterface( IID_IDispatch, ( void ** )&app );
                pUnk->Release();
            }
        }
    }
    ~ComThreadScope()
    {
        app.Release();
        if ( comInit )
        {
            CoUninitialize();
        }
    }
    bool isValid() const { return app != nullptr; }
};

QString ExtractBaseFolderName( const QString &folderName )
{
    int     pos  = folderName.indexOf( '_' );
    QString base = ( pos != -1 && pos <= 3 ) ? folderName.mid( pos + 1 ) : folderName;
    return base.replace( '_', ' ' );
}

void ScanFolderRecursive( OutlookManager *mgr, CComPtr<IDispatch> folder, const QString &currentPath, bool isTriageNode, bool isLearningNode, QJsonArray &nodesArray, int &learnedRules, int &misplacedItems, double oldestTimeStr )
{
    if ( !folder || !mgr )
    {
        return;
    }

    QString     rawName = mgr->GetFolderName( folder );
    QJsonObject nodeData;
    nodeData["folder_name"]     = currentPath.isEmpty() ? rawName : currentPath + " -> " + rawName;
    nodeData["has_issues"]      = isTriageNode;
    nodeData["allocated_items"] = 0;

    QSet<QString> uniqueSenders;
    QJsonArray    sendersArray;

    CComVariant itemsVar = mgr->GetProperty( folder, L"Items" );
    if ( itemsVar.vt == VT_DISPATCH && itemsVar.pdispVal )
    {
        CComVariant cVar            = mgr->GetProperty( itemsVar.pdispVal, L"Count" );
        long        count           = ( cVar.vt == VT_I4 ) ? cVar.lVal : 0;
        nodeData["allocated_items"] = ( qint64 )count;

        if ( isTriageNode )
        {
            misplacedItems += count;
        }

        if ( ( isTriageNode || isLearningNode ) && count > 0 )
        {
            long checkLimit = qMin( count, 500L );
            for ( long i = count; i > count - checkLimit; --i )
            {
                CComVariant idx( i );
                DISPPARAMS  p = { &idx, NULL, 1, 0 };
                CComVariant itm;
                if ( SUCCEEDED( mgr->InvokeMethod( itemsVar.pdispVal, L"Item", DISPATCH_METHOD, &itm, &p ) ) && itm.vt == VT_DISPATCH )
                {
                    if ( oldestTimeStr > 0 )
                    {
                        CComVariant tVar = mgr->GetProperty( itm.pdispVal, L"ReceivedTime" );
                        if ( tVar.vt == VT_DATE && tVar.date < oldestTimeStr )
                        {
                            continue;
                        }
                    }
                    QString sender = mgr->GetSenderAddress( itm.pdispVal );
                    if ( !sender.isEmpty() )
                    {
                        uniqueSenders.insert( sender );
                        if ( isLearningNode && mgr->m_rules )
                        {
                            ContactRule learnedContact;
                            learnedContact.name = ExtractBaseFolderName( rawName );
                            learnedContact.emails.append( sender );
                            mgr->m_rules->SaveContact( learnedContact );
                            learnedRules++;
                        }
                    }
                }
            }
        }
    }

    for ( const QString &s : uniqueSenders )
    {
        sendersArray.append( s );
    }
    nodeData["senders"] = sendersArray;

    if ( isTriageNode || currentPath.isEmpty() || nodeData["allocated_items"].toInt() > 0 )
    {
        nodesArray.append( nodeData );
    }

    CComVariant subFoldersVar = mgr->GetProperty( folder, L"Folders" );
    if ( subFoldersVar.vt == VT_DISPATCH && subFoldersVar.pdispVal )
    {
        CComVariant sfCountVar = mgr->GetProperty( subFoldersVar.pdispVal, L"Count" );
        if ( sfCountVar.vt == VT_I4 )
        {
            for ( long i = 1; i <= sfCountVar.lVal; ++i )
            {
                CComVariant index( i );
                DISPPARAMS  params = { &index, NULL, 1, 0 };
                CComVariant sfVar;
                if ( SUCCEEDED( mgr->InvokeMethod( subFoldersVar.pdispVal, L"Item", DISPATCH_METHOD, &sfVar, &params ) ) && sfVar.vt == VT_DISPATCH )
                {
                    ScanFolderRecursive( mgr, sfVar.pdispVal, currentPath.isEmpty() ? rawName : currentPath + " -> " + rawName, isTriageNode, isLearningNode, nodesArray, learnedRules, misplacedItems, oldestTimeStr );
                }
            }
        }
    }
}
} // namespace

void OutlookManager::ProcessInbox( bool fromUI, bool skipUnread )
{
    ComThreadScope scope;
    if ( !scope.isValid() )
    {
        return;
    }

    CComPtr<IDispatch> mapi;
    CComVariant        nsArg( L"MAPI" );
    DISPPARAMS         nsParams = { &nsArg, NULL, 1, 0 };
    CComVariant        nsRes;
    if ( FAILED( InvokeMethod( scope.app, L"GetNamespace", DISPATCH_METHOD, &nsRes, &nsParams ) ) || nsRes.vt != VT_DISPATCH )
    {
        return;
    }
    mapi = nsRes.pdispVal;

    CComVariant inboxArg( 6 );
    DISPPARAMS  inParams = { &inboxArg, NULL, 1, 0 };
    CComVariant inboxRes;
    if ( FAILED( InvokeMethod( mapi, L"GetDefaultFolder", DISPATCH_METHOD, &inboxRes, &inParams ) ) || inboxRes.vt != VT_DISPATCH )
    {
        return;
    }

    CComPtr<IDispatch> rootFolder;
    CComVariant        parentRes = GetProperty( inboxRes.pdispVal, L"Parent" );
    if ( parentRes.vt == VT_DISPATCH )
    {
        rootFolder = parentRes.pdispVal;
    }
    if ( !rootFolder )
    {
        return;
    }

    QString                       activeAccount = m_config->GetConfigString( "selected_account" );
    QHash<QString, QSet<QString>> sessionTargetCache;

    CComVariant inboxItemsVar = GetProperty( inboxRes.pdispVal, L"Items" );
    if ( inboxItemsVar.vt == VT_DISPATCH && inboxItemsVar.pdispVal )
    {
        CComVariant countVar = GetProperty( inboxItemsVar.pdispVal, L"Count" );
        if ( countVar.vt == VT_I4 && countVar.lVal > 0 )
        {
            for ( long i = countVar.lVal; i >= 1; --i )
            {
                CComVariant idx( i );
                DISPPARAMS  p = { &idx, NULL, 1, 0 };
                CComVariant itm;
                if ( SUCCEEDED( InvokeMethod( inboxItemsVar.pdispVal, L"Item", DISPATCH_METHOD, &itm, &p ) ) && itm.vt == VT_DISPATCH )
                {
                    CComVariant unreadVar = GetProperty( itm.pdispVal, L"Unread" );
                    if ( skipUnread && unreadVar.vt == VT_BOOL && unreadVar.boolVal )
                    {
                        continue;
                    }
                    RouteMailItem( itm.pdispVal, rootFolder, activeAccount, false, false, fromUI, sessionTargetCache );
                }
            }
        }
    }
}

bool OutlookManager::AuditFolders( bool maxTwoYears )
{
    ComThreadScope scope;
    if ( !scope.isValid() )
    {
        return false;
    }

    QJsonObject snapshot;
    QJsonArray  structuralNodes;
    int         totalLearned   = 0;
    int         totalMisplaced = 0;
    double      oldestTime     = 0.0;

    QJsonObject sysJson;
    m_config->LoadSystemFolders( sysJson );
    QJsonObject mapping = sysJson["entry_ids"].toObject();

    QSet<QString> auditNodeIds;
    QSet<QString> targetNodeIds;

    if ( mapping["Triage_Audit"].isArray() )
    {
        for ( const QJsonValue &val : mapping["Triage_Audit"].toArray() )
        {
            auditNodeIds.insert( val.toString() );
        }
    }

    if ( mapping["Triage_ReAudit"].isArray() )
    {
        for ( const QJsonValue &val : mapping["Triage_ReAudit"].toArray() )
        {
            auditNodeIds.insert( val.toString() );
        }
    }

    if ( mapping["Target_Organizations"].isArray() )
    {
        for ( const QJsonValue &val : mapping["Target_Organizations"].toArray() )
        {
            targetNodeIds.insert( val.toString() );
        }
    }

    if ( mapping["Target_Contacts"].isArray() )
    {
        for ( const QJsonValue &val : mapping["Target_Contacts"].toArray() )
        {
            targetNodeIds.insert( val.toString() );
        }
    }

    for ( const QString &entryId : auditNodeIds )
    {
        CComPtr<IDispatch> folder = GetFolderByEntryID( scope.app, entryId );
        if ( folder )
        {
            ScanFolderRecursive( this, folder, "", true, false, structuralNodes, totalLearned, totalMisplaced, oldestTime );
        }
    }

    for ( const QString &entryId : targetNodeIds )
    {
        CComPtr<IDispatch> folder = GetFolderByEntryID( scope.app, entryId );
        if ( folder )
        {
            ScanFolderRecursive( this, folder, "", false, true, structuralNodes, totalLearned, totalMisplaced, oldestTime );
        }
    }

    if ( m_rules )
    {
        m_rules->SaveAllRules();
    }

    snapshot["structural_nodes"]          = structuralNodes;
    snapshot["total_learned_rules"]       = totalLearned;
    snapshot["total_misplaced_items"]     = totalMisplaced;
    snapshot["has_overall_discrepancies"] = ( totalMisplaced > 0 );

    QString activeAccount = m_config->GetConfigString( "selected_account" );
    if ( !activeAccount.isEmpty() )
    {
        QString accountSafe = activeAccount;
        accountSafe.replace( "@", "_" ).replace( ".", "_" );
        QString reportDir = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/OutlookManager/Accounts/" + accountSafe;
        QDir().mkpath( reportDir );

        QFile file( reportDir + "/Wols_CA_InboxStatus_Snapshot.wolstriage" );
        if ( file.open( QIODevice::WriteOnly ) )
        {
            file.write( QJsonDocument( snapshot ).toJson( QJsonDocument::Indented ) );
            file.close();
        }
    }
    return true;
}

// --------------------------------------------------------------------------
// Routing Stubs
// --------------------------------------------------------------------------
CComPtr<IDispatch> OutlookManager::EnsureFolderExists( CComPtr<IDispatch> parentFolder, const QString &folderName ) { return nullptr; }
bool               OutlookManager::SafeMove( CComPtr<IDispatch> item, CComPtr<IDispatch> targetFolder, const QString &targetName, const QString &subj, double timeVal, QHash<QString, QSet<QString>> &sessionTargetCache ) { return false; }
bool               OutlookManager::RouteMailItem( CComPtr<IDispatch> mailItem, CComPtr<IDispatch> rootFolder, const QString &activeAccount, bool isSentItem, bool isAlreadyInQuarantine, bool fromUI, QHash<QString, QSet<QString>> &sessionTargetCache ) { return false; }
