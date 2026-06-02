#include "RuleEditorDialog.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMessageBox>
#include <QStringList>

RuleEditorDialog::RuleEditorDialog( RulesManager *rulesManager, QWidget *parent )
    : QDialog( parent ), m_rules( rulesManager ), m_currentOrgId( 0 ), m_currentContactId( 0 ), m_currentTenderId( 0 )
{
    setWindowTitle( "Wols CA - Rule Editor" );
    setMinimumSize( 800, 600 );

    setStyleSheet( R"(
        QDialog { background-color: #0f172a; } 
        QLabel { color: #f8fafc; font-weight: bold; }
        QLineEdit, QTextEdit, QListWidget { color: #f8fafc; background-color: #1e293b; border: 1px solid #334155; padding: 5px; }
        QPushButton { background-color: #334155; color: white; border-radius: 4px; padding: 6px 12px; font-weight: bold; }
        QPushButton:hover { background-color: #475569; }
        QPushButton#btnSave { background-color: #16a34a; }
        QPushButton#btnSave:hover { background-color: #15803d; }
        QTabBar::tab { background: #1e293b; color: #94a3b8; padding: 8px 16px; border: 1px solid #334155; }
        QTabBar::tab:selected { background: #0f172a; color: #38bdf8; border-bottom: 2px solid #38bdf8; }
        QTabWidget::pane { border: 1px solid #334155; }
    )" );

    setupUi();
    loadData();
}

RuleEditorDialog::~RuleEditorDialog() {}

void RuleEditorDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout( this );

    m_tabWidget = new QTabWidget( this );

    QWidget *tabOrgs = new QWidget();
    setupOrganizationsTab( tabOrgs );
    m_tabWidget->addTab( tabOrgs, "🏢 Organizations" );

    QWidget *tabContacts = new QWidget();
    setupContactsTab( tabContacts );
    m_tabWidget->addTab( tabContacts, "👤 Personal Contacts" );

    QWidget *tabTenders = new QWidget();
    setupTendersTab( tabTenders );
    m_tabWidget->addTab( tabTenders, "📄 Tenders" );

    mainLayout->addWidget( m_tabWidget );

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *btnClose     = new QPushButton( "Close", this );
    connect( btnClose, &QPushButton::clicked, this, &QDialog::accept );

    buttonLayout->addStretch();
    buttonLayout->addWidget( btnClose );
    mainLayout->addLayout( buttonLayout );
}

void RuleEditorDialog::loadData()
{
    m_listOrgs->clear();
    m_listContacts->clear();
    m_listTenders->clear();

    QString       jsonRaw = m_rules->GetRawRulesJSON();
    QJsonDocument doc     = QJsonDocument::fromJson( jsonRaw.toUtf8() );

    if ( doc.isObject() )
    {
        QJsonObject root = doc.object();

        if ( root.contains( "organizations" ) )
        {
            for ( const auto &val : root["organizations"].toArray() )
            {
                m_listOrgs->addItem( val.toObject()["name"].toString() );
            }
        }

        if ( root.contains( "contacts" ) )
        {
            for ( const auto &val : root["contacts"].toArray() )
            {
                m_listContacts->addItem( val.toObject()["name"].toString() );
            }
        }

        if ( root.contains( "tenders" ) )
        {
            for ( const auto &val : root["tenders"].toArray() )
            {
                m_listTenders->addItem( val.toObject()["name"].toString() );
            }
        }
    }
}

// ==============================================================================
// 1. ORGANISATIE TAB LOGICA
// ==============================================================================
void RuleEditorDialog::setupOrganizationsTab( QWidget *tab )
{
    QHBoxLayout *hLayout = new QHBoxLayout( tab );

    QVBoxLayout *listLayout = new QVBoxLayout();
    m_listOrgs              = new QListWidget( this );
    connect( m_listOrgs, &QListWidget::itemSelectionChanged, this, &RuleEditorDialog::onOrgSelectionChanged );

    QPushButton *btnNew = new QPushButton( "➕ New Organization", this );
    connect( btnNew, &QPushButton::clicked, this, &RuleEditorDialog::onNewOrgClicked );

    listLayout->addWidget( new QLabel( "Organizations:" ) );
    listLayout->addWidget( m_listOrgs );
    listLayout->addWidget( btnNew );
    hLayout->addLayout( listLayout, 1 );

    QFormLayout *formLayout = new QFormLayout();
    m_txtOrgName            = new QLineEdit( this );
    m_txtOrgLongName        = new QLineEdit( this );
    m_txtOrgFolder          = new QLineEdit( this );
    m_txtOrgDomains         = new QTextEdit( this );
    m_txtOrgDomains->setPlaceholderText( "E.g. @microsoft.com (One per line)" );

    formLayout->addRow( "Internal Name:", m_txtOrgName );
    formLayout->addRow( "Long Name:", m_txtOrgLongName );
    formLayout->addRow( "Folder Target:", m_txtOrgFolder );
    formLayout->addRow( "Domain Catchalls:", m_txtOrgDomains );

    QPushButton *btnSave = new QPushButton( "💾 Save Organization", this );
    btnSave->setObjectName( "btnSave" );
    connect( btnSave, &QPushButton::clicked, this, &RuleEditorDialog::onSaveOrgClicked );
    formLayout->addRow( "", btnSave );

    hLayout->addLayout( formLayout, 2 );
}

void RuleEditorDialog::onNewOrgClicked()
{
    m_listOrgs->clearSelection();
    m_currentOrgId = 0;
    m_txtOrgName->clear();
    m_txtOrgLongName->clear();
    m_txtOrgFolder->clear();
    m_txtOrgDomains->clear();
    m_txtOrgName->setFocus();
}

void RuleEditorDialog::onOrgSelectionChanged()
{
    if ( m_listOrgs->selectedItems().isEmpty() )
    {
        return;
    }
    QString selectedName = m_listOrgs->selectedItems().first()->text();

    QJsonDocument doc = QJsonDocument::fromJson( m_rules->GetRawRulesJSON().toUtf8() );
    if ( doc.isObject() && doc.object().contains( "organizations" ) )
    {
        for ( const QJsonValue &val : doc.object()["organizations"].toArray() )
        {
            QJsonObject orgObj = val.toObject();
            if ( orgObj["name"].toString() == selectedName )
            {
                m_currentOrgId = orgObj["id"].toVariant().toULongLong();
                m_txtOrgName->setText( orgObj["name"].toString() );
                m_txtOrgLongName->setText( orgObj["long_name"].toString() );
                m_txtOrgFolder->setText( orgObj["folder_id"].toString() );

                QStringList domains;
                for ( const auto &dom : orgObj["domain_catchall"].toArray() )
                {
                    domains << dom.toString();
                }
                m_txtOrgDomains->setText( domains.join( "\n" ) );
                break;
            }
        }
    }
}

void RuleEditorDialog::onSaveOrgClicked()
{
    if ( m_txtOrgName->text().isEmpty() )
    {
        QMessageBox::warning( this, "Error", "Organization name cannot be empty." );
        return;
    }

    OrganizationRule newOrg;
    newOrg.id       = m_currentOrgId;
    newOrg.name     = m_txtOrgName->text();
    newOrg.longName = m_txtOrgLongName->text();
    newOrg.folderId = m_txtOrgFolder->text();

    QStringList domains = m_txtOrgDomains->toPlainText().split( "\n", Qt::SkipEmptyParts );
    for ( const QString &d : domains )
    {
        newOrg.domainCatchall.append( d.trimmed() );
    }

    m_rules->SaveOrganization( newOrg );
    QMessageBox::information( this, "Success", "Organization saved successfully." );
    loadData();
}

// ==============================================================================
// 2. CONTACTS TAB LOGICA
// ==============================================================================
void RuleEditorDialog::setupContactsTab( QWidget *tab )
{
    QHBoxLayout *hLayout = new QHBoxLayout( tab );

    QVBoxLayout *listLayout = new QVBoxLayout();
    m_listContacts          = new QListWidget( this );
    connect( m_listContacts, &QListWidget::itemSelectionChanged, this, &RuleEditorDialog::onContactSelectionChanged );

    QPushButton *btnNew = new QPushButton( "➕ New Contact", this );
    connect( btnNew, &QPushButton::clicked, this, &RuleEditorDialog::onNewContactClicked );

    listLayout->addWidget( new QLabel( "Personal Contacts:" ) );
    listLayout->addWidget( m_listContacts );
    listLayout->addWidget( btnNew );
    hLayout->addLayout( listLayout, 1 );

    QFormLayout *formLayout = new QFormLayout();
    m_txtContactName        = new QLineEdit( this );
    m_txtContactLongName    = new QLineEdit( this );
    m_txtContactFolder      = new QLineEdit( this );
    m_txtContactEmails      = new QTextEdit( this );
    m_txtContactEmails->setPlaceholderText( "E.g. john.doe@gmail.com (One per line)" );

    formLayout->addRow( "Internal Name:", m_txtContactName );
    formLayout->addRow( "Long Name:", m_txtContactLongName );
    formLayout->addRow( "Folder Target:", m_txtContactFolder );
    formLayout->addRow( "Email Addresses:", m_txtContactEmails );

    QPushButton *btnSave = new QPushButton( "💾 Save Contact", this );
    btnSave->setObjectName( "btnSave" );
    connect( btnSave, &QPushButton::clicked, this, &RuleEditorDialog::onSaveContactClicked );
    formLayout->addRow( "", btnSave );

    hLayout->addLayout( formLayout, 2 );
}

void RuleEditorDialog::onNewContactClicked()
{
    m_listContacts->clearSelection();
    m_currentContactId = 0;
    m_txtContactName->clear();
    m_txtContactLongName->clear();
    m_txtContactFolder->clear();
    m_txtContactEmails->clear();
    m_txtContactName->setFocus();
}

void RuleEditorDialog::onContactSelectionChanged()
{
    if ( m_listContacts->selectedItems().isEmpty() )
    {
        return;
    }
    QString selectedName = m_listContacts->selectedItems().first()->text();

    QJsonDocument doc = QJsonDocument::fromJson( m_rules->GetRawRulesJSON().toUtf8() );
    if ( doc.isObject() && doc.object().contains( "contacts" ) )
    {
        for ( const QJsonValue &val : doc.object()["contacts"].toArray() )
        {
            QJsonObject cntObj = val.toObject();
            if ( cntObj["name"].toString() == selectedName )
            {
                m_currentContactId = cntObj["id"].toVariant().toULongLong();
                m_txtContactName->setText( cntObj["name"].toString() );
                m_txtContactLongName->setText( cntObj["long_name"].toString() );
                m_txtContactFolder->setText( cntObj["folder_id"].toString() );

                QStringList emails;
                for ( const auto &em : cntObj["emails"].toArray() )
                {
                    emails << em.toString();
                }
                m_txtContactEmails->setText( emails.join( "\n" ) );
                break;
            }
        }
    }
}

void RuleEditorDialog::onSaveContactClicked()
{
    if ( m_txtContactName->text().isEmpty() )
    {
        QMessageBox::warning( this, "Error", "Contact name cannot be empty." );
        return;
    }

    ContactRule newContact;
    newContact.id       = m_currentContactId;
    newContact.name     = m_txtContactName->text();
    newContact.longName = m_txtContactLongName->text();
    newContact.folderId = m_txtContactFolder->text();

    QStringList emails = m_txtContactEmails->toPlainText().split( "\n", Qt::SkipEmptyParts );
    for ( const QString &e : emails )
    {
        newContact.emails.append( e.trimmed() );
    }

    m_rules->SaveContact( newContact );
    QMessageBox::information( this, "Success", "Contact saved successfully." );
    loadData();
}

// ==============================================================================
// 3. TENDERS TAB LOGICA
// ==============================================================================
void RuleEditorDialog::setupTendersTab( QWidget *tab )
{
    QHBoxLayout *hLayout = new QHBoxLayout( tab );

    QVBoxLayout *listLayout = new QVBoxLayout();
    m_listTenders           = new QListWidget( this );
    connect( m_listTenders, &QListWidget::itemSelectionChanged, this, &RuleEditorDialog::onTenderSelectionChanged );

    QPushButton *btnNew = new QPushButton( "➕ New Tender", this );
    connect( btnNew, &QPushButton::clicked, this, &RuleEditorDialog::onNewTenderClicked );

    listLayout->addWidget( new QLabel( "Tenders:" ) );
    listLayout->addWidget( m_listTenders );
    listLayout->addWidget( btnNew );
    hLayout->addLayout( listLayout, 1 );

    QFormLayout *formLayout = new QFormLayout();
    m_txtTenderName         = new QLineEdit( this );
    m_txtTenderLongName     = new QLineEdit( this );
    m_txtTenderFolder       = new QLineEdit( this );
    m_txtTenderKeywords     = new QTextEdit( this );
    m_txtTenderKeywords->setPlaceholderText( "Keywords mapping to this tender (One per line)" );

    formLayout->addRow( "Internal Name:", m_txtTenderName );
    formLayout->addRow( "Long Name:", m_txtTenderLongName );
    formLayout->addRow( "Folder Target:", m_txtTenderFolder );
    formLayout->addRow( "Keywords:", m_txtTenderKeywords );

    QPushButton *btnSave = new QPushButton( "💾 Save Tender", this );
    btnSave->setObjectName( "btnSave" );
    connect( btnSave, &QPushButton::clicked, this, &RuleEditorDialog::onSaveTenderClicked );
    formLayout->addRow( "", btnSave );

    hLayout->addLayout( formLayout, 2 );
}

void RuleEditorDialog::onNewTenderClicked()
{
    m_listTenders->clearSelection();
    m_currentTenderId = 0;
    m_txtTenderName->clear();
    m_txtTenderLongName->clear();
    m_txtTenderFolder->clear();
    m_txtTenderKeywords->clear();
    m_txtTenderName->setFocus();
}

void RuleEditorDialog::onTenderSelectionChanged()
{
    if ( m_listTenders->selectedItems().isEmpty() )
    {
        return;
    }
    QString selectedName = m_listTenders->selectedItems().first()->text();

    QJsonDocument doc = QJsonDocument::fromJson( m_rules->GetRawRulesJSON().toUtf8() );
    if ( doc.isObject() && doc.object().contains( "tenders" ) )
    {
        for ( const QJsonValue &val : doc.object()["tenders"].toArray() )
        {
            QJsonObject tndObj = val.toObject();
            if ( tndObj["name"].toString() == selectedName )
            {
                m_currentTenderId = tndObj["id"].toVariant().toULongLong();
                m_txtTenderName->setText( tndObj["name"].toString() );
                m_txtTenderLongName->setText( tndObj["long_name"].toString() );
                m_txtTenderFolder->setText( tndObj["folder_id"].toString() );

                QStringList keywords;
                for ( const auto &kw : tndObj["keywords"].toArray() )
                {
                    keywords << kw.toString();
                }
                m_txtTenderKeywords->setText( keywords.join( "\n" ) );
                break;
            }
        }
    }
}

void RuleEditorDialog::onSaveTenderClicked()
{
    if ( m_txtTenderName->text().isEmpty() )
    {
        QMessageBox::warning( this, "Error", "Tender name cannot be empty." );
        return;
    }

    TenderRule newTender;
    newTender.id       = m_currentTenderId;
    newTender.name     = m_txtTenderName->text();
    newTender.longName = m_txtTenderLongName->text();
    newTender.folderId = m_txtTenderFolder->text();

    QStringList keywords = m_txtTenderKeywords->toPlainText().split( "\n", Qt::SkipEmptyParts );
    for ( const QString &k : keywords )
    {
        newTender.keywords.append( k.trimmed() );
    }

    m_rules->SaveTender( newTender );
    QMessageBox::information( this, "Success", "Tender saved successfully." );
    loadData();
}