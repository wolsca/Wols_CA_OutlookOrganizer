#include "MainWindow.hpp"
#include "AsyncWorker.hpp"
#include "FolderConfigDialog.hpp"
#include "RuleEditorDialog.hpp"

#include <QApplication>
#include <QGroupBox>
#include <QGuiApplication>
#include <QMessageBox>
#include <QMetaObject>
#include <QScreen>
#include <QTimer>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

MainWindow::MainWindow( ConfigManager *config, RulesManager *rules, OutlookManager *outlook, QWidget *parent )
    : QMainWindow( parent ), m_config( config ), m_rules( rules ), m_outlook( outlook )
{
    setWindowTitle( "Wols CA - Matrix Triage Control" );
    setMinimumSize( 950, 480 );

    setWindowIcon( QIcon( ":/Resource/Wols_CA_OutlookManager.ico" ) );

    setStyleSheet( R"(
        QMainWindow { background-color: #0f172a; } 
        QLabel { color: #f8fafc; font-weight: bold; font-size: 14px; }
        QGroupBox { color: #38bdf8; font-weight: bold; font-size: 14px; border: 1px solid #334155; margin-top: 15px; padding-top: 15px; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }
        
        QPushButton { background-color: #334155; color: white; border-radius: 4px; padding: 10px 15px; font-weight: bold; }
        QPushButton:hover { background-color: #475569; }
        QPushButton:disabled { background-color: #1e293b; color: #64748b; border: 1px solid #334155; }
        
        QPushButton#btnProcess { background-color: #16a34a; }
        QPushButton#btnProcess:hover { background-color: #15803d; }
        QPushButton#btnAudit { background-color: #2563eb; }
        QPushButton#btnAudit:hover { background-color: #1d4ed8; }
        QPushButton#btnRules { background-color: #4f46e5; }
        QPushButton#btnRules:hover { background-color: #4338ca; }
        
        QComboBox, QCheckBox { color: #ffffff; background-color: #1e293b; padding: 5px; border: 1px solid #334155; font-size: 13px; }
        QComboBox QAbstractItemView { background-color: #1e293b; color: #ffffff; selection-background-color: #38bdf8; selection-color: #0f172a; border: 1px solid #334155; outline: 0px; }
    )" );

    setupUi();
    setupTrayIcon();
    loadSettings();

    if ( m_chkMagnetize->isChecked() )
    {
        alignWithOutlook();
    }
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi()
{
    QWidget *centralWidget = new QWidget( this );
    setCentralWidget( centralWidget );
    QVBoxLayout *mainLayout = new QVBoxLayout( centralWidget );
    mainLayout->setSpacing( 25 );
    mainLayout->setContentsMargins( 25, 25, 25, 25 );

    QHBoxLayout *headerLayout = new QHBoxLayout();

    QLabel *lblTitle = new QLabel( "🛡️ Matrix Triage Control", this );
    lblTitle->setStyleSheet( "font-size: 24px; color: #38bdf8;" );

    QVBoxLayout *centerDisplay = new QVBoxLayout();
    centerDisplay->setSpacing( 5 );

    m_lblLogo = new QLabel( this );
    m_lblLogo->setAlignment( Qt::AlignCenter );
    m_lblLogo->setFixedSize( 120, 120 );

    m_pixLogo   = QPixmap( ":/Resource/Wols_CA_OutlookManager_Logo.bmp" ).scaled( 100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation );
    m_movieBusy = new QMovie( ":/Resource/Wols_CA_OutlookManager_Busy.gif", QByteArray(), this );
    m_movieBusy->setScaledSize( QSize( 80, 80 ) );

    m_lblLogo->setPixmap( m_pixLogo );

    m_lblStatus = new QLabel( "_ ready _", this );
    m_lblStatus->setAlignment( Qt::AlignCenter );
    m_lblStatus->setStyleSheet( "color: #94a3b8; font-family: monospace; font-size: 16px;" );

    centerDisplay->addWidget( m_lblLogo );
    centerDisplay->addWidget( m_lblStatus );

    QPushButton *btnShutdown = new QPushButton( "⏻", this );
    btnShutdown->setFixedSize( 40, 40 );
    btnShutdown->setStyleSheet( "background-color: #991b1b; border-radius: 4px; font-size: 20px;" );
    btnShutdown->setToolTip( "Shutdown Application" );

    headerLayout->addWidget( lblTitle, 1, Qt::AlignLeft | Qt::AlignTop );
    headerLayout->addLayout( centerDisplay, 2 );
    headerLayout->addWidget( btnShutdown, 1, Qt::AlignRight | Qt::AlignTop );

    mainLayout->addLayout( headerLayout );

    QGroupBox   *grpConfig    = new QGroupBox( "Systeem Configuratie", this );
    QHBoxLayout *configLayout = new QHBoxLayout( grpConfig );

    m_cboAccount = new QComboBox( this );
    for ( const QString &acc : m_outlook->GetAvailableAccounts() )
    {
        m_cboAccount->addItem( acc );
    }

    m_cboMode = new QComboBox( this );
    m_cboMode->addItem( "1. Fully Operational", static_cast<int>( TriageMode::FullyOperational ) );
    m_cboMode->addItem( "2. Make Rules Only", static_cast<int>( TriageMode::MakeRulesOnly ) );
    m_cboMode->addItem( "3. Apply All Rules Only", static_cast<int>( TriageMode::ApplyAllRulesOnly ) );
    m_cboMode->addItem( "4. Apply One Rule Only", static_cast<int>( TriageMode::ApplyOneRuleOnly ) );
    m_cboMode->addItem( "5. Remove Double Emails", static_cast<int>( TriageMode::RemoveDoubleEmails ) );
    m_cboMode->addItem( "6. Off (Standby)", static_cast<int>( TriageMode::Off ) );
    m_cboMode->addItem( "7. Audit Only", static_cast<int>( TriageMode::AuditOnly ) );

    m_chkUnread    = new QCheckBox( "Unread keep in Inbox", this );
    m_chkAutoStart = new QCheckBox( "Start on Boot", this );
    m_chkMagnetize = new QCheckBox( "Magnetize to Outlook", this );
    m_chkMagnetize->setToolTip( "Houdt dit venster strak naast Microsoft Outlook." );

    configLayout->addWidget( new QLabel( "Active Account:" ) );
    configLayout->addWidget( m_cboAccount );
    configLayout->addSpacing( 20 );
    configLayout->addWidget( new QLabel( "Operationele Modus:" ) );
    configLayout->addWidget( m_cboMode );
    configLayout->addStretch();
    configLayout->addWidget( m_chkUnread );
    configLayout->addSpacing( 10 );
    configLayout->addWidget( m_chkAutoStart );
    configLayout->addSpacing( 10 );
    configLayout->addWidget( m_chkMagnetize );

    mainLayout->addWidget( grpConfig );

    QGroupBox   *grpActions = new QGroupBox( "Engine Control", this );
    QGridLayout *gridLayout = new QGridLayout( grpActions );
    gridLayout->setSpacing( 15 );

    m_btnProcess = new QPushButton( "▶ Process Now", this );
    m_btnProcess->setObjectName( "btnProcess" );

    m_btnAuditShort = new QPushButton( "🔍 Audit (2 Years)", this );
    m_btnAuditShort->setObjectName( "btnAudit" );

    m_btnAuditLong = new QPushButton( "🔍 Audit (Full)", this );
    m_btnAuditLong->setObjectName( "btnAudit" );

    m_btnBackupManager = new QPushButton( "💾 Backup Manager", this );

    gridLayout->addWidget( m_btnProcess, 0, 0 );
    gridLayout->addWidget( m_btnAuditShort, 0, 1 );
    gridLayout->addWidget( m_btnAuditLong, 0, 2 );
    gridLayout->addWidget( m_btnBackupManager, 0, 3 );

    m_btnSelectProcessFolders = new QPushButton( "📁 Select Process Folders", this );
    m_btnSelectAuditFolders   = new QPushButton( "📁 Select Audit Folders", this );

    m_btnRules = new QPushButton( "📝 Rule Editor", this );
    m_btnRules->setObjectName( "btnRules" );

    m_btnDedouble = new QPushButton( "✂️ Dedouble", this );

    gridLayout->addWidget( m_btnSelectProcessFolders, 1, 0 );
    gridLayout->addWidget( m_btnSelectAuditFolders, 1, 1 );
    gridLayout->addWidget( m_btnRules, 1, 2 );
    gridLayout->addWidget( m_btnDedouble, 1, 3 );

    mainLayout->addWidget( grpActions );
    mainLayout->addStretch();

    connect( btnShutdown, &QPushButton::clicked, qApp, &QApplication::quit );
    connect( m_btnProcess, &QPushButton::clicked, this, &MainWindow::onProcessClicked );
    connect( m_btnAuditShort, &QPushButton::clicked, this, &MainWindow::onAuditShortClicked );
    connect( m_btnAuditLong, &QPushButton::clicked, this, &MainWindow::onAuditLongClicked );
    connect( m_btnDedouble, &QPushButton::clicked, this, &MainWindow::onDedoubleClicked );

    connect( m_btnSelectProcessFolders, &QPushButton::clicked, this, &MainWindow::onSelectProcessFoldersClicked );
    connect( m_btnSelectAuditFolders, &QPushButton::clicked, this, &MainWindow::onSelectAuditFoldersClicked );
    connect( m_btnBackupManager, &QPushButton::clicked, this, &MainWindow::onBackupManagerClicked );
    connect( m_btnRules, &QPushButton::clicked, this, &MainWindow::onOpenRulesEditor );

    connect( m_cboMode, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &MainWindow::onModeChanged );
    connect( m_cboAccount, &QComboBox::currentTextChanged, this, &MainWindow::onAccountChanged );
    connect( m_chkAutoStart, &QCheckBox::toggled, this, &MainWindow::onAutoStartToggled );
    connect( m_chkUnread, &QCheckBox::toggled, this, &MainWindow::onUnreadToggled );
    connect( m_chkMagnetize, &QCheckBox::toggled, this, &MainWindow::onMagnetizeToggled );
}

void MainWindow::setupTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon( this );
    QIcon appIcon( ":/Resource/Wols_CA_OutlookManager.ico" );
    if ( appIcon.isNull() )
    {
        appIcon = style()->standardIcon( QStyle::SP_ComputerIcon );
    }

    m_trayIcon->setIcon( appIcon );
    m_trayIcon->setToolTip( "Wols CA - Matrix Triage Control" );

    m_trayMenu          = new QMenu( this );
    QAction *showAction = new QAction( "Dashboard Tonen", this );
    connect( showAction, &QAction::triggered, this, &MainWindow::showNormal );
    m_trayMenu->addAction( showAction );
    m_trayMenu->addSeparator();
    QAction *quitAction = new QAction( "Afsluiten", this );
    connect( quitAction, &QAction::triggered, qApp, &QApplication::quit );
    m_trayMenu->addAction( quitAction );

    m_trayIcon->setContextMenu( m_trayMenu );
    m_trayIcon->show();

    connect( m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated );
}

void MainWindow::onTrayIconActivated( QSystemTrayIcon::ActivationReason reason )
{
    if ( reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger )
    {
        showNormal();
        activateWindow();
    }
}

void MainWindow::closeEvent( QCloseEvent *event )
{
    if ( m_trayIcon->isVisible() )
    {
        hide();
        event->ignore();
    }
}

void MainWindow::loadSettings()
{
    QString savedAccount = m_config->GetConfigString( "selected_account" );
    if ( !savedAccount.isEmpty() )
    {
        m_cboAccount->setCurrentText( savedAccount );
    }

    int savedMode = m_config->GetConfigInt( "triage_mode", static_cast<int>( TriageMode::Off ) );
    int index     = m_cboMode->findData( savedMode );
    if ( index != -1 )
    {
        m_cboMode->setCurrentIndex( index );
    }

    m_chkAutoStart->setChecked( m_config->GetRegistryBool( "auto_start", true ) );
    m_chkUnread->setChecked( m_config->GetRegistryBool( "process_unread_only", true ) );
    m_chkMagnetize->setChecked( m_config->GetRegistryBool( "magnetize_window", true ) );
}

void MainWindow::setBusyState( bool isBusy, const QString &statusText )
{
    updateStatus( statusText, isBusy ? "#eab308" : "#94a3b8" );

    if ( isBusy )
    {
        m_lblLogo->setMovie( m_movieBusy );
        m_movieBusy->start();
    }
    else
    {
        m_movieBusy->stop();
        m_lblLogo->setPixmap( m_pixLogo );
    }

    m_btnProcess->setEnabled( !isBusy );
    m_btnAuditShort->setEnabled( !isBusy );
    m_btnAuditLong->setEnabled( !isBusy );
    m_btnDedouble->setEnabled( !isBusy );
}

void MainWindow::updateStatus( const QString &text, const QString &color )
{
    m_lblStatus->setText( text );
    m_lblStatus->setStyleSheet( QString( "color: %1; font-family: monospace; font-size: 16px;" ).arg( color ) );
}

void MainWindow::onProcessClicked()
{
    TriageMode currentMode = static_cast<TriageMode>( m_cboMode->currentData().toInt() );

    // GUARD CLAUSE: Verwerk geen data als de app uit staat of puur op audit staat.
    if ( currentMode == TriageMode::AuditOnly || currentMode == TriageMode::Off )
    {
        QMessageBox::warning( this,
                              "Process Disabled",
                              "De applicatie staat in 'Off' of 'Audit Only' modus.\nEr kunnen geen e-mails worden verwerkt of verplaatst." );
        return;
    }

    setBusyState( true, "_ processing _" );
    bool skipUnread = m_chkUnread->isChecked();

    AsyncWorker::Run( this, [this, skipUnread]()
                      { m_outlook->ProcessInbox( true, skipUnread ); }, [this]()
                      { setBusyState( false, "_ ready _" ); } );
}

void MainWindow::onAuditShortClicked()
{
    TriageMode currentMode = static_cast<TriageMode>( m_cboMode->currentData().toInt() );

    // GUARD CLAUSE: Audit mag niet draaien als de app volledig uitgeschakeld is.
    if ( currentMode == TriageMode::Off )
    {
        QMessageBox::information( this,
                                  "Engine Offline",
                                  "De applicatie staat ingesteld op 'Off (Standby)'.\nPas de modus aan om een audit te starten." );
        return;
    }

    setBusyState( true, "_ auditing (max 2 yr) _" );

    AsyncWorker::Run( this, [this]()
                      { const bool ok = m_outlook->AuditFolders( true ); }, [this]()
                      { setBusyState( false, "_ ready _" ); } );
}

void MainWindow::onAuditLongClicked()
{
    TriageMode currentMode = static_cast<TriageMode>( m_cboMode->currentData().toInt() );

    // GUARD CLAUSE
    if ( currentMode == TriageMode::Off )
    {
        QMessageBox::information( this,
                                  "Engine Offline",
                                  "De applicatie staat ingesteld op 'Off (Standby)'.\nPas de modus aan om een audit te starten." );
        return;
    }

    setBusyState( true, "_ auditing (full) _" );

    AsyncWorker::Run( this, [this]()
                      { m_outlook->PerformDeepAudit(); }, [this]()
                      { setBusyState( false, "_ ready _" ); } );
}

void MainWindow::onDedoubleClicked()
{
    TriageMode currentMode = static_cast<TriageMode>( m_cboMode->currentData().toInt() );

    // GUARD CLAUSE: Deduplicatie wijzigt data, dus blokkeer dit in veilige modi.
    if ( currentMode == TriageMode::AuditOnly || currentMode == TriageMode::Off )
    {
        QMessageBox::warning( this,
                              "Action Disabled",
                              "De applicatie staat in 'Off' of 'Audit Only' modus.\nEr kunnen geen duplicaten worden verwijderd in deze status." );
        return;
    }

    setBusyState( true, "_ dedoubling _" );
    AsyncWorker::Run( this, [this]()
                      { std::this_thread::sleep_for( std::chrono::seconds( 2 ) ); }, [this]()
                      { setBusyState( false, "_ ready _" ); } );
}

void MainWindow::onModeChanged( int index ) { m_config->SetConfigString( "triage_mode", QString::number( m_cboMode->itemData( index ).toInt() ) ); }
void MainWindow::onAccountChanged( const QString &account )
{
    m_config->SetConfigString( "selected_account", account );
    m_rules->LoadForAccount( account );
}
void MainWindow::onAutoStartToggled( bool checked ) { m_config->SetRegistryBool( "auto_start", checked ); }
void MainWindow::onUnreadToggled( bool checked ) { m_config->SetRegistryBool( "process_unread_only", checked ); }

void MainWindow::onOpenRulesEditor()
{
    RuleEditorDialog dialog( m_rules, this );
    dialog.exec();
}

void MainWindow::alignWithOutlook()
{
    HWND hwnd = FindWindowW( L"rctrl_renwnd32", NULL );
    if ( hwnd )
    {
        if ( IsIconic( hwnd ) || !IsWindowVisible( hwnd ) )
        {
            return;
        }

        RECT rect;
        if ( GetWindowRect( hwnd, &rect ) )
        {
            int outX     = rect.left;
            int outY     = rect.top;
            int outWidth = rect.right - rect.left;

            int targetX = outX + outWidth + 15;
            int targetY = outY;

            QScreen *screen = QGuiApplication::screenAt( QPoint( outX, outY ) );
            if ( !screen )
            {
                screen = QGuiApplication::primaryScreen();
            }

            if ( screen )
            {
                QRect screenGeom = screen->availableGeometry();
                if ( targetX + this->width() > screenGeom.right() )
                {
                    targetX = outX - this->width() - 15;

                    if ( targetX < screenGeom.left() )
                    {
                        targetX = screenGeom.center().x() - ( this->width() / 2 );
                        targetY = screenGeom.center().y() - ( this->height() / 2 );
                    }
                }
            }
            this->move( targetX, targetY );
        }
    }
}

void MainWindow::onMagnetizeToggled( bool checked )
{
    m_config->SetRegistryBool( "magnetize_window", checked );
    if ( checked )
    {
        alignWithOutlook();
    }
}

void MainWindow::changeEvent( QEvent *event )
{
    if ( event->type() == QEvent::ActivationChange && this->isActiveWindow() )
    {
        if ( m_chkMagnetize->isChecked() )
        {
            alignWithOutlook();
        }
    }
    QMainWindow::changeEvent( event );
}

void MainWindow::onSelectProcessFoldersClicked()
{
    QList<QPair<QString, QString>> fields = {
        { "Target_Organizations", "📁 Target Organizations Node" },
        { "Target_Contacts", "📁 Target Contacts Node" } };

    FolderConfigDialog dialog( m_config, m_outlook, "Process Architecture Focus", fields, this );
    dialog.exec();
}

void MainWindow::onSelectAuditFoldersClicked()
{
    QList<QPair<QString, QString>> fields = {
        { "Triage_Audit", "🔍 Audit Holding Node" },
        { "Triage_ReAudit", "🔍 Re-Audit Holding Node" } };

    FolderConfigDialog dialog( m_config, m_outlook, "Audit Architecture Focus", fields, this );
    dialog.exec();
}

void MainWindow::onBackupManagerClicked()
{
    QMessageBox::information( this, "Backup Manager", "Backup beheer scherm komt in de volgende fase." );
}