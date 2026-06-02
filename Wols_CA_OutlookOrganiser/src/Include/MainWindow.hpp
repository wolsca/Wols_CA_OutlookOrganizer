#pragma once

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMovie>
#include <QPixmap>
#include <QPushButton>
#include <QSystemTrayIcon>
#include <QVBoxLayout>

#include "ConfigManager.hpp"
#include "OutlookManager.hpp"
#include "RulesManager.hpp"

// The 6 operational states for the routing engine
enum class TriageMode
{
    FullyOperational   = 1,
    MakeRulesOnly      = 2,
    ApplyAllRulesOnly  = 3,
    ApplyOneRuleOnly   = 4,
    RemoveDoubleEmails = 5,
    Off                = 6
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    MainWindow( ConfigManager *config, RulesManager *rules, OutlookManager *outlook, QWidget *parent = nullptr );
    ~MainWindow();

  protected:
    void closeEvent( QCloseEvent *event ) override;

    // Qt focus handler and our alignment function
    void changeEvent( QEvent *event ) override;
    void alignWithOutlook();

  private slots:
    // Core Engine Actions
    void onProcessClicked();
    void onAuditShortClicked();
    void onAuditLongClicked();
    void onDedoubleClicked();

    // UI Configuration Actions
    void onModeChanged( int index );
    void onAccountChanged( const QString &account );
    void onAutoStartToggled( bool checked );
    void onUnreadToggled( bool checked );
    void onMagnetizeToggled( bool checked );

    // Open Sub-screens / Selections
    void onOpenRulesEditor();
    void onSelectProcessFoldersClicked();
    void onSelectAuditFoldersClicked();
    void onBackupManagerClicked();

    // System Tray
    void onTrayIconActivated( QSystemTrayIcon::ActivationReason reason );

  private:
    void setupUi();
    void setupTrayIcon();
    void loadSettings();

    // Visual status & GIF logic
    void setBusyState( bool isBusy, const QString &statusText );
    void updateStatus( const QString &text, const QString &color = "#94a3b8" );

    ConfigManager  *m_config;
    RulesManager   *m_rules;
    OutlookManager *m_outlook;

    // --- UI Elements ---
    QSystemTrayIcon *m_trayIcon;
    QMenu           *m_trayMenu;

    QComboBox *m_cboAccount;
    QComboBox *m_cboMode;
    QCheckBox *m_chkUnread;
    QCheckBox *m_chkAutoStart;
    QCheckBox *m_chkMagnetize;

    // Header & Animation
    QLabel *m_lblLogo;
    QLabel *m_lblStatus;
    QPixmap m_pixLogo;
    QMovie *m_movieBusy;

    // Buttons Grid
    QPushButton *m_btnProcess;
    QPushButton *m_btnSelectProcessFolders;

    QPushButton *m_btnAuditShort;
    QPushButton *m_btnSelectAuditFolders;

    QPushButton *m_btnAuditLong;
    QPushButton *m_btnRules;

    QPushButton *m_btnBackupManager;
    QPushButton *m_btnDedouble;
};