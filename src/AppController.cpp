#include "AppController.h"
#include <QAction>
#include <algorithm>
#include <QApplication>
#include <QDir>
#include <QJsonValue>
#include <QMenu>
#include <QQuickWindow>
#include <QSystemTrayIcon>
#include <QUrl>

AppController::AppController(QString projectRoot, QObject *parent, bool enableTray)
    : QObject(parent), m_engine(this),
      m_store(QDir(projectRoot).filePath("data/state.json")),
      m_sound(QDir(projectRoot).filePath("data/audio-log.jsonl"), this),
      m_displayedDate(QDate::currentDate())
{
    connect(&m_sound, &ChimePlayer::errorChanged, this, &AppController::errorChanged);
    connect(&m_engine, &TimerEngine::checkpointRequested, this, [this] {
        if (!m_loading) save();
    });
    connect(&m_engine, &TimerEngine::changed, this, &AppController::updateTray);
    connect(&m_engine, &TimerEngine::stageCompleted, this, &AppController::completeStage);
    m_dateRefresh.setInterval(1000);
    connect(&m_dateRefresh, &QTimer::timeout, this, [this] {
        const auto current = QDate::currentDate();
        if (current != m_displayedDate) {
            m_displayedDate = current;
            emit statisticsChanged();
        }
    });
    m_dateRefresh.start();
    if (enableTray && QSystemTrayIcon::isSystemTrayAvailable()) {
        m_tray = new QSystemTrayIcon(applicationIcon(), this);
        m_trayMenu = std::make_unique<QMenu>();
        m_trayMenu->addAction(QStringLiteral("显示番茄钟"), this, &AppController::showWindow);
        m_toggleAction = m_trayMenu->addAction(QStringLiteral("开始专注"), this, &AppController::primaryAction);
        m_trayMenu->addSeparator();
        m_trayMenu->addAction(QStringLiteral("退出"), this, &AppController::quit);
        m_tray->setContextMenu(m_trayMenu.get());
        connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) showWindow();
        });
        connect(m_tray, &QSystemTrayIcon::messageClicked, this, &AppController::showWindow);
        m_tray->show();
        updateTray();
    }
}
AppController::~AppController()
{
    if (m_tray) {
        m_tray->hide();
        m_tray->setContextMenu(nullptr);
    }
}
QIcon AppController::applicationIcon()
{
    return QIcon(QStringLiteral(":/assets/app.ico"));
}
bool AppController::initialize(QString &error)
{
    QJsonObject document;
    bool exists = false;
    if (!m_store.read(document, exists, error)) return false;
    if (exists) {
        if (document.value("schemaVersion").toInt(-1) != 1
            || !document.value("settings").isObject()
            || !document.value("timer").isObject()
            || !document.value("statistics").isObject()) {
            error = QStringLiteral("数据文件版本或结构无效：%1\n原文件已保留。").arg(m_store.filePath());
            return false;
        }
        PomodoroSettings savedSettings;
        if (!PomodoroSettings::fromJson(document.value("settings").toObject(), savedSettings, error))
            return false;
        if (!m_statistics.fromJson(document.value("statistics").toObject(), error)) return false;
        m_engine.setSettings(savedSettings);
        if (!m_engine.restore(document.value("timer").toObject(), error)) return false;
    }
    m_loading = false;
    emit settingsChanged();
    emit statisticsChanged();
    updateTray();
    save();
    return true;
}
void AppController::attachWindow(QQuickWindow *window)
{
    m_window = window;
    m_restoreMaximized = window->visibility() == QWindow::Maximized;
    connect(window, &QWindow::visibilityChanged, this, [this](QWindow::Visibility visibility) {
        if (visibility == QWindow::Maximized) m_restoreMaximized = true;
        else if (visibility == QWindow::Windowed) m_restoreMaximized = false;
    });
    applyWindowFlags();
}
bool AppController::save()
{
    if (m_loading) return false;
    const QJsonObject document{{"schemaVersion", 1}, {"settings", m_engine.settings().toJson()},
        {"timer", m_engine.snapshot()}, {"statistics", m_statistics.toJson()}};
    QString error;
    const bool success = m_store.write(document, error);
    if (error != m_storageError) {
        m_storageError = error;
        emit errorChanged();
    }
    return success;
}
void AppController::primaryAction()
{
    if (m_engine.state() == TimerEngine::Running) m_engine.pause();
    else m_engine.start();
}
void AppController::requestReset()
{
    if (m_engine.requiresConfirmation()) {
        emit confirmationRequested(QStringLiteral("重置这一段？"),
            QStringLiteral("当前进度将被放弃，不会计入完成统计。"), 0, m_engine.phase());
    } else m_engine.reset();
}
void AppController::requestPhase(int phase)
{
    if (phase < TimerEngine::Focus || phase > TimerEngine::LongBreak || (phase == m_engine.phase() && m_engine.state() != TimerEngine::Finished)) return;
    if (m_engine.requiresConfirmation()) {
        emit confirmationRequested(QStringLiteral("切换阶段？"),
            QStringLiteral("当前进度将被放弃，新的阶段会等待你点击开始。"), 1, phase);
    } else m_engine.selectPhase(phase);
}
void AppController::confirmAction(int action, int phase)
{
    if (action == 0) m_engine.reset();
    else if (action == 1) m_engine.selectPhase(phase);
}
bool AppController::applySettings(const QVariantMap &values)
{
    PomodoroSettings candidate;
    QString error;
    if (!PomodoroSettings::fromJson(QJsonObject::fromVariantMap(values), candidate, error)) {
        m_validationError = error;
        emit errorChanged();
        return false;
    }
    m_validationError.clear();
    m_engine.setSettings(candidate);
    if (!candidate.soundEnabled) {
        m_sound.stop();
    }
    applyWindowFlags();
    emit settingsChanged();
    emit errorChanged();
    return save();
}
void AppController::playChime(int volume)
{
    m_sound.play(volume, QStringLiteral("stage-completed-%1").arg(m_engine.phase()));
}
void AppController::previewSound(int volume)
{
    m_sound.play(volume, QStringLiteral("preview"));
}
void AppController::toggleOnTop()
{
    auto candidate = m_engine.settings();
    candidate.alwaysOnTop = !candidate.alwaysOnTop;
    m_engine.setSettings(candidate);
    applyWindowFlags();
    emit settingsChanged();
    save();
}
void AppController::applyWindowFlags()
{
    if (!m_window) return;
    auto flags = m_window->flags();
    if (alwaysOnTop()) flags |= Qt::WindowStaysOnTopHint;
    else flags &= ~Qt::WindowStaysOnTopHint;
    m_window->setFlags(flags);
}
void AppController::updateTray()
{
    if (!m_tray) return;
    m_tray->setToolTip(QStringLiteral("番茄钟 · %1 %2\n%3")
        .arg(m_engine.phaseName(), m_engine.displayTime(), m_engine.statusText()));
    m_toggleAction->setText(m_engine.primaryLabel());
}
void AppController::completeStage(int phase, qint64 duration, const QDateTime &finishedAt)
{
    if (phase == TimerEngine::Focus) {
        m_statistics.addFocus(duration, finishedAt);
        emit statisticsChanged();
    }
    // Persist the completed state and its statistic before issuing the alert.
    save();
    if (m_engine.settings().soundEnabled) playChime(m_engine.settings().volume);
    if (m_tray && QSystemTrayIcon::supportsMessages())
        m_tray->showMessage(QStringLiteral("%1完成").arg(m_engine.phaseName()),
            m_engine.statusText() + QStringLiteral("\n点击打开，下一阶段由你开始。"),
            QSystemTrayIcon::Information, 8000);
    if (m_window) m_window->alert(0);
}
void AppController::showWindow()
{
    if (!m_window) return;
    if (m_restoreMaximized) m_window->showMaximized();
    else m_window->showNormal();
    m_window->raise();
    m_window->requestActivate();
}
bool AppController::closeWindow()
{
    if (!m_tray) {
        quit();
        return true;
    }
    if (m_window) m_window->hide();
    if (m_firstHide) {
        m_firstHide = false;
        m_tray->showMessage(QStringLiteral("番茄钟仍在运行"),
            QStringLiteral("计时和提醒会继续。点击托盘图标可打开，右键菜单可退出。"));
    }
    save();
    return false;
}
void AppController::suspend()
{
    m_engine.pause(QStringLiteral("电脑进入睡眠，已暂停；准备好时点击继续"));
    m_sound.stop(QStringLiteral("system-suspend"));
}
void AppController::quit()
{
    m_engine.pause();
    save();
    m_sound.stop(QStringLiteral("application-quit"));
    QApplication::quit();
}
