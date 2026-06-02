#include "OutlookManager.hpp"
#include <QDebug>

namespace
{
QString HrToHex( HRESULT hr )
{
    return QString( "0x%1" ).arg( static_cast<qulonglong>( static_cast<unsigned long>( hr ) ), 8, 16, QLatin1Char( '0' ) ).toUpper();
}
} // namespace

OutlookManager::OutlookManager( ConfigManager *config, RulesManager *rules )
    : m_config( config ), m_rules( rules )
{
}

OutlookManager::~OutlookManager()
{
}

HRESULT OutlookManager::InvokeMethod( IDispatch *pDisp, LPCOLESTR name, WORD wFlags, VARIANT *pVarResult, DISPPARAMS *pParams )
{
    if ( !pDisp )
    {
        return E_FAIL;
    }
    DISPID  dispid;
    HRESULT hr = pDisp->GetIDsOfNames( IID_NULL, const_cast<LPOLESTR *>( &name ), 1, LOCALE_USER_DEFAULT, &dispid );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    DISPPARAMS emptyParams = { NULL, NULL, 0, 0 };
    return pDisp->Invoke( dispid, IID_NULL, LOCALE_USER_DEFAULT, wFlags, pParams ? pParams : &emptyParams, pVarResult, NULL, NULL );
}

CComVariant OutlookManager::GetProperty( IDispatch *pDisp, LPCOLESTR name )
{
    CComVariant result;
    if ( pDisp )
    {
        DISPID dispid;
        if ( SUCCEEDED( pDisp->GetIDsOfNames( IID_NULL, const_cast<LPOLESTR *>( &name ), 1, LOCALE_USER_DEFAULT, &dispid ) ) )
        {
            DISPPARAMS emptyParams = { NULL, NULL, 0, 0 };
            pDisp->Invoke( dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &emptyParams, &result, NULL, NULL );
        }
    }
    return result;
}

HRESULT OutlookManager::PutProperty( IDispatch *pDisp, LPCOLESTR name, VARIANT *pVar )
{
    if ( !pDisp )
    {
        return E_FAIL;
    }
    DISPID  dispid;
    HRESULT hr = pDisp->GetIDsOfNames( IID_NULL, const_cast<LPOLESTR *>( &name ), 1, LOCALE_USER_DEFAULT, &dispid );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    DISPID     dispidPut = DISPID_PROPERTYPUT;
    DISPPARAMS params    = { pVar, &dispidPut, 1, 1 };
    return pDisp->Invoke( dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &params, NULL, NULL, NULL );
}

CComPtr<IDispatch> OutlookManager::GetFolderByEntryID( CComPtr<IDispatch> app, const QString &entryIdHex )
{
    if ( entryIdHex.isEmpty() || !app )
    {
        return nullptr;
    }

    CComPtr<IDispatch> mapi;
    CComVariant        nsArg( L"MAPI" );
    DISPPARAMS         nsParams = { &nsArg, NULL, 1, 0 };
    CComVariant        nsRes;

    if ( SUCCEEDED( InvokeMethod( app, L"GetNamespace", DISPATCH_METHOD, &nsRes, &nsParams ) ) && nsRes.vt == VT_DISPATCH )
    {
        mapi = nsRes.pdispVal;
        CComVariant idArg( entryIdHex.toStdWString().c_str() );
        DISPPARAMS  getParams = { &idArg, NULL, 1, 0 };
        CComVariant folderRes;

        if ( SUCCEEDED( InvokeMethod( mapi, L"GetFolderFromID", DISPATCH_METHOD, &folderRes, &getParams ) ) && folderRes.vt == VT_DISPATCH )
        {
            return folderRes.pdispVal;
        }
    }
    return nullptr;
}