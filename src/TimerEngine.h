#pragma once
#include <QObject>
#include <QDateTime>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QTimer>
#include <functional>

struct PomodoroSettings {
    int focusMinutes = 25;
    int shortMinutes = 5;
    int longMinutes = 15;
    int cycles = 4;
    int volume = 70;
    bool soundEnabled = true;
    bool alwaysOnTop = false;
    QJsonObject toJson() const;
    static bool fromJson(const QJsonObject &json, PomodoroSettings &result, QString &error);
};

class TimerEngine final : public QObject {
    Q_OBJECT
    Q_PROPERTY(int phase READ phase NOTIFY changed)
    Q_PROPERTY(int state READ state NOTIFY changed)
    Q_PROPERTY(int remainingSeconds READ remainingSeconds NOTIFY changed)
    Q_PROPERTY(QString displayTime READ displayTime NOTIFY changed)
    Q_PROPERTY(double progress READ progress NOTIFY changed)
    Q_PROPERTY(QString phaseName READ phaseName NOTIFY changed)
    Q_PROPERTY(QString statusText READ statusText NOTIFY changed)
    Q_PROPERTY(QString primaryLabel READ primaryLabel NOTIFY changed)
    Q_PROPERTY(QString nextPhaseName READ nextPhaseName NOTIFY changed)
    Q_PROPERTY(int completedInCycle READ completedInCycle NOTIFY changed)
    Q_PROPERTY(int cycleTarget READ cycleTarget NOTIFY changed)
public:
    enum Phase { Focus, ShortBreak, LongBreak };
    Q_ENUM(Phase)
    enum State { Ready, Running, Paused, Finished };
    Q_ENUM(State)
    using Clock = std::function<qint64()>;
    using DateClock = std::function<QDateTime()>;
    explicit TimerEngine(QObject *parent = nullptr, Clock clock = {}, DateClock dateClock = {});
    int phase() const { return m_phase; }
    int state() const { return m_state; }
    int remainingSeconds() const;
    QString displayTime() const;
    double progress() const;
    QString phaseName() const;
    QString statusText() const;
    QString primaryLabel() const;
    QString nextPhaseName() const;
    int completedInCycle() const { return m_completedInCycle; }
    int cycleTarget() const { return m_settings.cycles; }
    bool requiresConfirmation() const;
    const PomodoroSettings &settings() const { return m_settings; }
    void setSettings(const PomodoroSettings &settings);
    void start();
    void pause(const QString &reason = {});
    void reset();
    void selectPhase(int phase);
    void tick();
    QJsonObject snapshot() const;
    bool restore(const QJsonObject &json, QString &error);
signals:
    void changed();
    void checkpointRequested();
    void stageCompleted(int phase, qint64 durationMs, const QDateTime &finishedAt);
private:
    qint64 elapsedMs() const;
    qint64 durationFor(Phase phase) const;
    QString nameFor(Phase phase) const;
    void prepare(Phase phase);
    void finish();
    PomodoroSettings m_settings;
    Phase m_phase = Focus;
    Phase m_nextPhase = ShortBreak;
    State m_state = Ready;
    qint64 m_durationMs = 25 * 60 * 1000;
    qint64 m_elapsedMs = 0;
    qint64 m_startedAt = 0;
    int m_completedInCycle = 0;
    QString m_pauseReason;
    QElapsedTimer m_monotonicClock;
    Clock m_clock;
    DateClock m_dateClock;
    QTimer m_refresh;
    QTimer m_checkpoint;
};

