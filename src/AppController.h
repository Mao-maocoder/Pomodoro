#pragma once
#include "TimerEngine.h"
#include "DataStore.h"
#include "DailyStatistics.h"
#include <QIcon>
#include "ChimePlayer.h"
#include <QVariantMap>
#include <memory>
class QQuickWindow;
class QSystemTrayIcon;
class QMenu;
class QAction;

class AppController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(TimerEngine *engine READ engine CONSTANT)
    Q_PROPERTY(QVariantMap settings READ settings NOTIFY settingsChanged)
    Q_PROPERTY(int todayCount READ todayCount NOTIFY statisticsChanged)
    Q_PROPERTY(qint64 todayMinutes READ todayMinutes NOTIFY statisticsChanged)
    Q_PROPERTY(QVariantList history READ history NOTIFY statisticsChanged)
    Q_PROPERTY(QString storageError READ storageError NOTIFY errorChanged)
    Q_PROPERTY(QString soundError READ soundError NOTIFY errorChanged)
    Q_PROPERTY(QString validationError READ validationError NOTIFY errorChanged)
    Q_PROPERTY(bool trayAvailable READ trayAvailable CONSTANT)
    Q_PROPERTY(bool alwaysOnTop READ alwaysOnTop NOTIFY settingsChanged)
public:
    explicit AppController(QString projectRoot, QObject *parent = nullptr, bool enableTray = true);
    ~AppController() override;
    TimerEngine *engine() { return &m_engine; }
    QVariantMap settings() const { return m_engine.settings().toJson().toVariantMap(); }
    int todayCount() const { return m_statistics.total(QDate::currentDate()).count; }
    qint64 todayMinutes() const { return m_statistics.total(QDate::currentDate()).durationMs / 60000; }
    QVariantList history() const { return m_statistics.recent(QDate::currentDate()); }
    QString storageError() const { return m_storageError; }
    QString soundError() const { return m_sound.errorString(); }
    QString validationError() const { return m_validationError; }
    bool trayAvailable() const { return m_tray != nullptr; }
    bool alwaysOnTop() const { return m_engine.settings().alwaysOnTop; }
    bool initialize(QString &error);
    void attachWindow(QQuickWindow *window);
    void suspend();
    bool save();
    static QIcon applicationIcon();
    Q_INVOKABLE void primaryAction();
    Q_INVOKABLE void requestReset();
    Q_INVOKABLE void requestPhase(int phase);
    Q_INVOKABLE void confirmAction(int action, int phase);
    Q_INVOKABLE bool applySettings(const QVariantMap &settings);
    Q_INVOKABLE void previewSound(int volume);
    Q_INVOKABLE void toggleOnTop();
    Q_INVOKABLE void showWindow();
    Q_INVOKABLE bool closeWindow();
    Q_INVOKABLE void quit();
signals:
    void settingsChanged();
    void statisticsChanged();
    void errorChanged();
    void confirmationRequested(const QString &title, const QString &message, int action, int phase);
private:
    void updateTray();
    void applyWindowFlags();
    void playChime(int volume);
    void completeStage(int phase, qint64 duration, const QDateTime &finishedAt);
    TimerEngine m_engine;
    DataStore m_store;
    DailyStatistics m_statistics;
    ChimePlayer m_sound;
    QTimer m_dateRefresh;
    QDate m_displayedDate;
    QQuickWindow *m_window = nullptr;
    QSystemTrayIcon *m_tray = nullptr;
    std::unique_ptr<QMenu> m_trayMenu;
    QAction *m_toggleAction = nullptr;
    QString m_storageError;
    QString m_validationError;
    bool m_loading = true;
    bool m_firstHide = true;
    bool m_restoreMaximized = false;
};

