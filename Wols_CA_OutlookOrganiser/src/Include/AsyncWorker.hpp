#pragma once

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QThreadPool> // Toegevoegd voor Qt's thread beheer

class AsyncWorker
{
  public:
    /**
     * Executes a background task using Qt's thread pool and safely returns execution to the main UI thread.
     * @param context    The UI object (like MainWindow) to receive the callback.
     * @param bgTask     The heavy operation to run on the background thread.
     * @param uiCallback The UI update to run when the background task finishes.
     */
    template <typename BackgroundTask, typename UiCallback>
    static void Run( QObject *context, BackgroundTask bgTask, UiCallback uiCallback )
    {
        // QPointer zorgt ervoor dat we niet crashen als de UI (zoals een dialoog)
        // al is afgesloten voordat de achtergrondtaak klaar is.
        QPointer<QObject> safeContext( context );

        // We gooien de lambda nu in de Qt Thread Pool in plaats van een ruwe std::thread
        QThreadPool::globalInstance()->start( [safeContext, bgTask, uiCallback]()
                                              {
                                                  // 1. Voer de zware Outlook logica uit (buiten de main thread)
                                                  bgTask();

                                                  // 2. Spring veilig terug naar de Qt main thread, ALLEEN als de UI nog bestaat
                                                  if ( safeContext )
                                                  {
                                                      QMetaObject::invokeMethod( safeContext, uiCallback, Qt::QueuedConnection );
                                                  } } );
    }
};