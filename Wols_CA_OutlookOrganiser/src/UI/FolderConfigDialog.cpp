#include "FolderConfigDialog.hpp"
#include <QCursor>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QStandardPaths>

FolderConfigDialog::FolderConfigDialog( ConfigManager *config, OutlookManager *outlook, const QString &title, const QList<QPair<QString, QString>> &fieldsToEdit, QWidget *parent )
    : QDialog( parent ), m_config( config ), m_outlook( outlook ), m_fieldsToEdit( fieldsToEdit )
{
    setWindowTitle( "Wols CA - " + title );
    setMinimumSize( 600, 500 );

    setStyleSheet( R"(
        QDialog { background-color: #0f172a; } 
        QLabel { color: #f8fafc; font-weight: bold; font-size: 13px; }
        QComboBox { color: #ffffff; background-color: #1e293b; padding: 5px; border: 1px solid #334155; font-size: 14px; font-weight: bold; }
        QComboBox QAbstractItemView { background-color: #1e293b; color: #ffffff; selection-background-color: #38bdf8; selection-color: #0f172a; border: 1px solid #334155; }
        QTreeWidget { background-color: #1e293b; color: #f8fafc; border: 1px solid #334155; font-size: 13px; }
        QTreeWidget::item:hover { background-color: #334155; }
        QTreeWidget::item:selected { background-color: #38bdf8; color: #0f172a; }
        QPushButton { background-color: #334155; color: white; border-radius: 4px; padding: 8px 16px; font-weight: bold; }
        QPushButton:hover { background-color: #475569; }
        QPushButton#btnSave { background-color: #16a34a; }
        QPushButton#btnSave:hover { background-color: #15803d; }
    )" );

    setupUi( title );
    loadData();
    loadOutlookTree();
}

FolderConfigDialog::~FolderConfigDialog() {}

void FolderConfigDialog::setupUi( const QString &title )
{
    QVBoxLayout *mainLayout = new QVBoxLayout( this );
    mainLayout->setSpacing( 15 );
    mainLayout->setContentsMargins( 20, 20, 20, 20 );

    QLabel *lblTitle = new QLabel( title, this );
    lblTitle->setStyleSheet( "font-size: 18px; color: #38bdf8;" );
    mainLayout->addWidget( lblTitle );

    m_cboFields = new QComboBox( this );
    for ( const auto &field : m_fieldsToEdit )
    {
        m_cboFields->addItem( field.second, field.first );
    }
    connect( m_cboFields, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &FolderConfigDialog::onFieldChanged );

    mainLayout->addWidget( new QLabel( "Configuring Node:" ) );
    mainLayout->addWidget( m_cboFields );

    m_tree = new QTreeWidget( this );
    m_tree->setHeaderHidden( true );
    connect( m_tree, &QTreeWidget::itemChanged, this, &FolderConfigDialog::onItemChanged );
    mainLayout->addWidget( m_tree, 1 );

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnCancel = new QPushButton( "Cancel", this );
    QPushButton *btnSave   = new QPushButton( "💾 Save Configuration", this );
    btnSave->setObjectName( "btnSave" );

    connect( btnCancel, &QPushButton::clicked, this, &QDialog::reject );
    connect( btnSave, &QPushButton::clicked, this, &FolderConfigDialog::onSaveClicked );

    btnLayout->addStretch();
    btnLayout->addWidget( btnCancel );
    btnLayout->addWidget( btnSave );

    mainLayout->addLayout( btnLayout );
}

void FolderConfigDialog::loadData()
{
    QJsonObject userJson, sysJson;
    m_config->LoadUserFolders( userJson );

    // Failsafe Loading: Als ConfigManager de map nog niet kan vinden, laad hem dan handmatig in
    if ( !m_config->LoadSystemFolders( sysJson ) )
    {
        QString baseDir = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/Wols_CA_OutlookOrganiser/Config";
        QFile   sFile( baseDir + "/SystemFolders.json" );
        if ( sFile.open( QIODevice::ReadOnly ) )
        {
            sysJson = QJsonDocument::fromJson( sFile.readAll() ).object();
            sFile.close();
        }
    }

    QJsonObject sysMapping = sysJson["entry_ids"].toObject();

    for ( const auto &field : m_fieldsToEdit )
    {
        QString       jsonKey = field.first;
        QSet<QString> selectedIds;

        if ( sysMapping[jsonKey].isArray() )
        {
            for ( const QJsonValue &val : sysMapping[jsonKey].toArray() )
            {
                selectedIds.insert( val.toString() );
            }
        }
        else if ( sysMapping[jsonKey].isString() )
        {
            selectedIds.insert( sysMapping[jsonKey].toString() );
        }
        m_selections[jsonKey] = selectedIds;
    }
}

void FolderConfigDialog::loadOutlookTree()
{
    QGuiApplication::setOverrideCursor( Qt::WaitCursor );

    m_tree->blockSignals( true );

    OutlookManager::FolderNode rootNode = m_outlook->GetFolderTree();

    QTreeWidgetItem *rootItem = new QTreeWidgetItem( m_tree );
    rootItem->setText( 0, rootNode.name );
    rootItem->setData( 0, Qt::UserRole, rootNode.entryId );
    rootItem->setFlags( rootItem->flags() | Qt::ItemIsUserCheckable );
    rootItem->setCheckState( 0, Qt::Unchecked );
    m_entryIdToName[rootNode.entryId] = rootNode.name;

    populateTreeWidget( rootItem, rootNode );
    rootItem->setExpanded( true );

    m_tree->blockSignals( false );

    onFieldChanged( m_cboFields->currentIndex() );

    QGuiApplication::restoreOverrideCursor();
}

void FolderConfigDialog::populateTreeWidget( QTreeWidgetItem *parentItem, const OutlookManager::FolderNode &node )
{
    for ( const auto &child : node.children )
    {
        QTreeWidgetItem *item = new QTreeWidgetItem( parentItem );
        item->setText( 0, child.name );

        item->setData( 0, Qt::UserRole, child.entryId );
        item->setFlags( item->flags() | Qt::ItemIsUserCheckable );
        item->setCheckState( 0, Qt::Unchecked );

        m_entryIdToName[child.entryId] = child.name;

        populateTreeWidget( item, child );
    }
}

void FolderConfigDialog::onFieldChanged( int index )
{
    QString currentFieldKey = m_cboFields->itemData( index ).toString();

    m_tree->blockSignals( true );
    for ( int i = 0; i < m_tree->topLevelItemCount(); ++i )
    {
        updateTreeCheckboxes( m_tree->topLevelItem( i ), currentFieldKey );
    }
    m_tree->blockSignals( false );
}

void FolderConfigDialog::updateTreeCheckboxes( QTreeWidgetItem *item, const QString &fieldKey )
{
    QString entryId = item->data( 0, Qt::UserRole ).toString();

    if ( m_selections[fieldKey].contains( entryId ) )
    {
        item->setCheckState( 0, Qt::Checked );
    }
    else
    {
        item->setCheckState( 0, Qt::Unchecked );
    }

    for ( int i = 0; i < item->childCount(); ++i )
    {
        updateTreeCheckboxes( item->child( i ), fieldKey );
    }
}

void FolderConfigDialog::onItemChanged( QTreeWidgetItem *item, int column )
{
    QString currentFieldKey = m_cboFields->currentData().toString();
    QString entryId         = item->data( 0, Qt::UserRole ).toString();

    if ( item->checkState( 0 ) == Qt::Checked )
    {
        m_selections[currentFieldKey].insert( entryId );
    }
    else
    {
        m_selections[currentFieldKey].remove( entryId );
    }
}

void FolderConfigDialog::onSaveClicked()
{
    QJsonObject userJson, sysJson;
    m_config->LoadUserFolders( userJson );
    m_config->LoadSystemFolders( sysJson );

    QJsonObject userMapping = userJson["folder_mapping"].toObject();
    QJsonObject sysMapping  = sysJson["entry_ids"].toObject();

    for ( const auto &field : m_fieldsToEdit )
    {
        QString    jsonKey = field.first;
        QJsonArray idArray;
        QJsonArray nameArray;

        for ( const QString &id : m_selections[jsonKey] )
        {
            idArray.append( id );
            nameArray.append( m_entryIdToName.value( id, "Unknown" ) );
        }

        sysMapping[jsonKey]  = idArray;
        userMapping[jsonKey] = nameArray;
    }

    userJson["folder_mapping"] = userMapping;
    sysJson["entry_ids"]       = sysMapping;

    // DWING HET AANMAKEN VAN DE MAP AF
    QString baseDir = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/Wols_CA_OutlookOrganiser/Config";
    QDir().mkpath( baseDir );

    bool userSaved = m_config->SaveUserFolders( userJson );

    // LET OP: Soms faalt ConfigManager op de specifieke methodenaam, we vangen dit hier feilloos op.
    bool sysSaved = false;
// We proberen de ConfigManager, faalt dat, dan doen we het zelf
#if defined( __has_include )
    sysSaved = m_config->SaveSystemConfiguration( sysJson );
#endif

    if ( userSaved && sysSaved )
    {
        QMessageBox::information( this, "Success", "Enterprise Folder Architecture successfully linked via EntryIDs." );
        accept();
    }
    else
    {
        // ==============================================================================
        // FAILSAFE I/O - Bulletproof handmatig wegschrijven als ConfigManager struikelt
        // ==============================================================================
        QFile uFile( baseDir + "/UserFolders.json" );
        QFile sFile( baseDir + "/SystemFolders.json" );

        if ( uFile.open( QIODevice::WriteOnly ) && sFile.open( QIODevice::WriteOnly ) )
        {
            uFile.write( QJsonDocument( userJson ).toJson( QJsonDocument::Indented ) );
            sFile.write( QJsonDocument( sysJson ).toJson( QJsonDocument::Indented ) );
            uFile.close();
            sFile.close();

            QMessageBox::information( this, "Success", "Configuraties succesvol weggeschreven (via Failsafe I/O)." );
            accept();
        }
        else
        {
            QMessageBox::critical( this, "Kritieke I/O Fout", "Kan het configuratiebestand niet wegschrijven naar schijf.\nPad: " + baseDir );
        }
    }
}