#pragma once

#include <QDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>

#include "RulesManager.hpp"

class RuleEditorDialog : public QDialog
{
    Q_OBJECT

  public:
    RuleEditorDialog( RulesManager *rulesManager, QWidget *parent = nullptr );
    ~RuleEditorDialog();

  private slots:
    void loadData();

    // Organisatie Slots
    void onOrgSelectionChanged();
    void onSaveOrgClicked();
    void onNewOrgClicked();

    // Contact Slots
    void onContactSelectionChanged();
    void onSaveContactClicked();
    void onNewContactClicked();

    // Tender Slots
    void onTenderSelectionChanged();
    void onSaveTenderClicked();
    void onNewTenderClicked();

  private:
    void setupUi();
    void setupOrganizationsTab( QWidget *tab );
    void setupContactsTab( QWidget *tab );
    void setupTendersTab( QWidget *tab );

    RulesManager *m_rules;

    // --- UI Elementen ---
    QTabWidget *m_tabWidget;

    // Organisatie Elementen
    QListWidget *m_listOrgs;
    QLineEdit   *m_txtOrgName;
    QLineEdit   *m_txtOrgLongName;
    QLineEdit   *m_txtOrgFolder;
    QTextEdit   *m_txtOrgDomains;
    uint64_t     m_currentOrgId;

    // Contact Elementen
    QListWidget *m_listContacts;
    QLineEdit   *m_txtContactName;
    QLineEdit   *m_txtContactLongName;
    QLineEdit   *m_txtContactFolder;
    QTextEdit   *m_txtContactEmails;
    uint64_t     m_currentContactId;

    // Tender Elementen
    QListWidget *m_listTenders;
    QLineEdit   *m_txtTenderName;
    QLineEdit   *m_txtTenderLongName;
    QLineEdit   *m_txtTenderFolder;
    QTextEdit   *m_txtTenderKeywords;
    uint64_t     m_currentTenderId;
};