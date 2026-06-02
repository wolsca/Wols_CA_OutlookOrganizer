#pragma once

#include <QComboBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QMap>
#include <QPair>
#include <QPushButton>
#include <QSet>
#include <QString>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "ConfigManager.hpp"
#include "OutlookManager.hpp"

class FolderConfigDialog : public QDialog
{
    Q_OBJECT

  public:
    FolderConfigDialog( ConfigManager *config, OutlookManager *outlook, const QString &title, const QList<QPair<QString, QString>> &fieldsToEdit, QWidget *parent = nullptr );
    ~FolderConfigDialog();

  private slots:
    void onSaveClicked();
    void onFieldChanged( int index );
    void onItemChanged( QTreeWidgetItem *item, int column );

  private:
    void setupUi( const QString &title );
    void loadData();
    void loadOutlookTree();
    void populateTreeWidget( QTreeWidgetItem *parentItem, const OutlookManager::FolderNode &node );
    void updateTreeCheckboxes( QTreeWidgetItem *item, const QString &fieldKey );

    ConfigManager  *m_config;
    OutlookManager *m_outlook;

    QList<QPair<QString, QString>> m_fieldsToEdit;

    // UI Elements
    QComboBox   *m_cboFields;
    QTreeWidget *m_tree;

    // Data Storage (JSON Key -> Set of EntryIDs)
    QMap<QString, QSet<QString>> m_selections;
    QMap<QString, QString>       m_entryIdToName;
};