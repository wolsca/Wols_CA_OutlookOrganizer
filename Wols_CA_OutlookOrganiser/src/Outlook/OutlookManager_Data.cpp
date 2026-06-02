#include "OutlookManager.hpp"
#include <QDebug>
#include <algorithm>

namespace
{
// Isolated COM scope for data retrieval
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
} // namespace

QList<QString> OutlookManager::GetAvailableAccounts()
{
    QList<QString> accounts;
    ComThreadScope scope;
    if ( !scope.isValid() )
    {
        return accounts;
    }

    CComVariant nsRes;
    CComVariant nsArg( L"MAPI" );
    DISPPARAMS  nsParams = { &nsArg, nullptr, 1, 0 };
    if ( FAILED( InvokeMethod( scope.app, L"GetNamespace", DISPATCH_METHOD, &nsRes, &nsParams ) ) || nsRes.vt != VT_DISPATCH || !nsRes.pdispVal )
    {
        return accounts;
    }

    CComVariant storesVar = GetProperty( nsRes.pdispVal, L"Folders" );
    if ( storesVar.vt != VT_DISPATCH || !storesVar.pdispVal )
    {
        return accounts;
    }

    CComVariant countVar = GetProperty( storesVar.pdispVal, L"Count" );
    if ( countVar.vt != VT_I4 )
    {
        return accounts;
    }

    for ( long i = 1; i <= countVar.lVal; ++i )
    {
        CComVariant idx( i );
        DISPPARAMS  p = { &idx, nullptr, 1, 0 };
        CComVariant folderRes;
        if ( SUCCEEDED( InvokeMethod( storesVar.pdispVal, L"Item", DISPATCH_METHOD, &folderRes, &p ) ) && folderRes.vt == VT_DISPATCH && folderRes.pdispVal )
        {
            QString name = GetFolderName( folderRes.pdispVal );
            if ( !name.isEmpty() )
            {
                accounts.append( name );
            }
        }
    }

    std::sort( accounts.begin(), accounts.end(), []( const QString &a, const QString &b )
               { return a.compare( b, Qt::CaseInsensitive ) < 0; } );
    return accounts;
}

OutlookManager::FolderNode OutlookManager::GetFolderTree()
{
    FolderNode rootNode;
    rootNode.name = "Mailbox Root";

    ComThreadScope scope;
    if ( !scope.isValid() )
    {
        return rootNode;
    }

    CComPtr<IDispatch> mapi;
    CComVariant        nsArg( L"MAPI" );
    DISPPARAMS         nsParams = { &nsArg, NULL, 1, 0 };
    CComVariant        nsRes;

    if ( FAILED( InvokeMethod( scope.app, L"GetNamespace", DISPATCH_METHOD, &nsRes, &nsParams ) ) || nsRes.vt != VT_DISPATCH )
    {
        return rootNode;
    }
    mapi = nsRes.pdispVal;

    CComVariant inboxArg( 6 );
    DISPPARAMS  inParams = { &inboxArg, NULL, 1, 0 };
    CComVariant inboxRes;
    if ( FAILED( InvokeMethod( mapi, L"GetDefaultFolder", DISPATCH_METHOD, &inboxRes, &inParams ) ) || inboxRes.vt != VT_DISPATCH )
    {
        return rootNode;
    }

    CComVariant parentRes = GetProperty( inboxRes.pdispVal, L"Parent" );
    if ( parentRes.vt == VT_DISPATCH && parentRes.pdispVal )
    {
        rootNode.name    = GetFolderName( parentRes.pdispVal );
        rootNode.entryId = GetEntryID( parentRes.pdispVal );
        PopulateFolderNode( parentRes.pdispVal, rootNode );
    }
    return rootNode;
}

void OutlookManager::PopulateFolderNode( CComPtr<IDispatch> folder, FolderNode &node )
{
    CComVariant foldersVar = GetProperty( folder, L"Folders" );
    if ( foldersVar.vt == VT_DISPATCH && foldersVar.pdispVal )
    {
        CComVariant countVar = GetProperty( foldersVar.pdispVal, L"Count" );
        if ( countVar.vt == VT_I4 && countVar.lVal > 0 )
        {
            for ( long i = 1; i <= countVar.lVal; ++i )
            {
                CComVariant index( i );
                DISPPARAMS  p = { &index, NULL, 1, 0 };
                CComVariant fVar;
                if ( SUCCEEDED( InvokeMethod( foldersVar.pdispVal, L"Item", DISPATCH_METHOD, &fVar, &p ) ) && fVar.vt == VT_DISPATCH )
                {
                    FolderNode child;
                    child.name    = GetFolderName( fVar.pdispVal );
                    child.entryId = GetEntryID( fVar.pdispVal );
                    if ( !child.name.isEmpty() )
                    {
                        PopulateFolderNode( fVar.pdispVal, child );
                        node.children.append( child );
                    }
                }
            }
        }
    }
}

CComPtr<IDispatch> OutlookManager::PickFolder()
{
    ComThreadScope scope;
    if ( !scope.isValid() )
    {
        return nullptr;
    }

    CComPtr<IDispatch> mapi;
    CComVariant        nsArg( L"MAPI" );
    DISPPARAMS         nsParams = { &nsArg, NULL, 1, 0 };
    CComVariant        nsRes;

    if ( SUCCEEDED( InvokeMethod( scope.app, L"GetNamespace", DISPATCH_METHOD, &nsRes, &nsParams ) ) && nsRes.vt == VT_DISPATCH )
    {
        mapi = nsRes.pdispVal;
        CComVariant folderRes;

        if ( SUCCEEDED( InvokeMethod( mapi, L"PickFolder", DISPATCH_METHOD, &folderRes, NULL ) ) && folderRes.vt == VT_DISPATCH )
        {
            return folderRes.pdispVal;
        }
    }
    return nullptr;
}

QString OutlookManager::GetFolderName( CComPtr<IDispatch> folder )
{
    CComVariant var = GetProperty( folder, L"Name" );
    return ( var.vt == VT_BSTR && var.bstrVal ) ? QString::fromWCharArray( var.bstrVal ) : "Unknown";
}

QString OutlookManager::GetEntryID( CComPtr<IDispatch> folder )
{
    if ( !folder )
    {
        return "";
    }
    CComVariant var = GetProperty( folder, L"EntryID" );
    return ( var.vt == VT_BSTR && var.bstrVal ) ? QString::fromWCharArray( var.bstrVal ) : "";
}

QString OutlookManager::GetMailSubject( CComPtr<IDispatch> mailItem )
{
    CComVariant var = GetProperty( mailItem, L"Subject" );
    return ( var.vt == VT_BSTR && var.bstrVal ) ? QString::fromWCharArray( var.bstrVal ) : "No Subject";
}

QString OutlookManager::GetMailBody( CComPtr<IDispatch> mailItem )
{
    CComVariant var = GetProperty( mailItem, L"Body" );
    return ( var.vt == VT_BSTR && var.bstrVal ) ? QString::fromWCharArray( var.bstrVal ) : "";
}

QString OutlookManager::GetSenderAddress( CComPtr<IDispatch> mailItem )
{
    if ( !mailItem )
    {
        return "";
    }
    CComVariant typeVar = GetProperty( mailItem, L"SenderEmailType" );
    QString     typeStr = ( typeVar.vt == VT_BSTR && typeVar.bstrVal ) ? QString::fromWCharArray( typeVar.bstrVal ) : "";

    QString finalAddress = "";
    if ( typeStr == "EX" )
    {
        CComVariant senderVar = GetProperty( mailItem, L"Sender" );
        if ( senderVar.vt == VT_DISPATCH && senderVar.pdispVal )
        {
            CComVariant exUserVar;
            if ( SUCCEEDED( InvokeMethod( senderVar.pdispVal, L"GetExchangeUser", DISPATCH_METHOD, &exUserVar, NULL ) ) && exUserVar.vt == VT_DISPATCH && exUserVar.pdispVal )
            {
                CComVariant smtpVar = GetProperty( exUserVar.pdispVal, L"PrimarySmtpAddress" );
                if ( smtpVar.vt == VT_BSTR && smtpVar.bstrVal )
                {
                    finalAddress = QString::fromWCharArray( smtpVar.bstrVal );
                }
            }
        }
    }
    if ( finalAddress.isEmpty() )
    {
        CComVariant addrVar = GetProperty( mailItem, L"SenderEmailAddress" );
        if ( addrVar.vt == VT_BSTR && addrVar.bstrVal )
        {
            finalAddress = QString::fromWCharArray( addrVar.bstrVal );
        }
    }
    return finalAddress;
}