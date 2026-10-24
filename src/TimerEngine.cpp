#include "TimerEngine.h"
#include <QJsonValue>
#include <algorithm>
namespace {
bool readInt(const QJsonObject &json, const QString &key, int minimum, int maximum, int &out)
{
    const auto value = json.value(key);
    if (!value.isDouble()) return false;
    const double number = value.toDouble();
    if (number < minimum || number > maximum || number != static_cast<int>(number)) return false;
    out = static_cast<int>(number);
    return true;
}
bool readMs(const QJsonObject &json, const QString &key, qint64 minimum, qint64 maximum, qint64 &out)
{
    const auto value = json.value(key);
    if (!value.isDouble()) return false;
    const double number = value.toDouble();
    if (number < minimum || number > maximum || number != static_cast<qint64>(number)) return false;
    out = static_cast<qint64>(number);
    return true;
}
}
QJsonObject PomodoroSettings::toJson() const
{
    return {{"focusMinutes", focusMinutes}, {"shortMinutes", shortMinutes},
            {"longMinutes", longMinutes}, {"cycles", cycles}, {"volume", volume},
            {"soundEnabled", soundEnabled}, {"alwaysOnTop", alwaysOnTop}};
}
bool PomodoroSettings::fromJson(const QJsonObject &json, PomodoroSettings &result, QString &error)
{
    PomodoroSettings candidate;
    if (!readInt(json, "focusMinutes", 1, 180, candidate.focusMinutes)
        || !readInt(json, "shortMinutes", 1, 60, candidate.shortMinutes)
        || !readInt(json, "longMinutes", 1, 120, candidate.longMinutes)
        || !readInt(json, "cycles", 1, 12, candidate.cycles)
        || !readInt(json, "volume", 0, 100, candidate.volume)
        || !json.value("soundEnabled").isBool() || !json.value("alwaysOnTop").isBool()) {
        error = QStringLiteral("设置格式无效：请检查时长、循环次数、音量和开关。");
        return false;
    }
    candidate.soundEnabled = json.value("soundEnabled").toBool();
    candidate.alwaysOnTop = json.value("alwaysOnTop").toBool();
    result = candidate;
    return true;
}
TimerEngine::TimerEngine(QObject *parent, Clock clock, DateClock dateClock)
    : QObject(parent), m_clock(std::move(clock)), m_dateClock(std::move(dateClock))
{
    m_monotonicClock.start();
    if (!m_clock) m_clock = [this] { return m_monotonicClock.elapsed(); };
    if (!m_dateClock) m_dateClock = [] { return QDateTime::currentDateTime(); };
    m_refresh.setInterval(250);
    m_refresh.setTimerType(Qt::PreciseTimer);
    connect(&m_refresh, &QTimer::timeout, this, &TimerEngine::tick);
    m_checkpoint.setInterval(10000);
    connect(&m_checkpoint, &QTimer::timeout, this, [this] {
        tick();
        if (m_state == Running) emit checkpointRequested();
    });
}
qint64 TimerEngine::elapsedMs() const
{
    const qint64 live = m_state == Running ? std::max<qint64>(0, m_clock() - m_startedAt) : 0;
    return std::clamp(m_elapsedMs + live, qint64(0), m_durationMs);
}
int TimerEngine::remainingSeconds() const
{
    return static_cast<int>((m_durationMs - elapsedMs() + 999) / 1000);
}
QString TimerEngine::displayTime() const
{
    const int seconds = remainingSeconds();
    return QString("%1:%2").arg(seconds / 60, 2, 10, QLatin1Char('0'))
        .arg(seconds % 60, 2, 10, QLatin1Char('0'));
}
double TimerEngine::progress() const
{
    return 1.0 - static_cast<double>(elapsedMs()) / static_cast<double>(m_durationMs);
}
QString TimerEngine::nameFor(Phase phase) const
{
    switch (phase) {
    case Focus: return QStringLiteral("专注");
    case ShortBreak: return QStringLiteral("短休息");
    case LongBreak: return QStringLiteral("长休息");
    }
    Q_UNREACHABLE();
}
QString TimerEngine::phaseName() const { return nameFor(m_phase); }
QString TimerEngine::nextPhaseName() const { return nameFor(m_nextPhase); }
QString TimerEngine::statusText() const
{
    switch (m_state) {
    case Ready:
        return m_phase == Focus ? QStringLiteral("为一件事，留一段完整的时间")
                                : QStringLiteral("放松一下，让注意力慢慢回来");
    case Running:
        return m_phase == Focus ? QStringLiteral("专注当下，一次只做一件事")
                                : QStringLiteral("起身走走，给自己一点空白");
    case Paused:
        return m_pauseReason.isEmpty() ? QStringLiteral("已暂停，准备好时继续") : m_pauseReason;
    case Finished:
        return m_phase == Focus ? QStringLiteral("这次专注完成了，休息一下吧")
                                : QStringLiteral("休息结束，开始下一段专注吧");
    }
    Q_UNREACHABLE();
}
QString TimerEngine::primaryLabel() const
{
    switch (m_state) {
    case Ready: return QStringLiteral("开始%1").arg(phaseName());
    case Running: return QStringLiteral("暂停");
    case Paused: return QStringLiteral("继续%1").arg(phaseName());
    case Finished: return QStringLiteral("开始%1").arg(nextPhaseName());
    }
    Q_UNREACHABLE();
}
qint64 TimerEngine::durationFor(Phase phase) const
{
    switch (phase) {
    case Focus: return qint64(m_settings.focusMinutes) * 60000;
    case ShortBreak: return qint64(m_settings.shortMinutes) * 60000;
    case LongBreak: return qint64(m_settings.longMinutes) * 60000;
    }
    Q_UNREACHABLE();
}
bool TimerEngine::requiresConfirmation() const
{
    return m_state == Running || (m_state == Paused && m_elapsedMs > 0);
}
void TimerEngine::setSettings(const PomodoroSettings &settings)
{
    m_settings = settings;
    if (m_state == Ready) m_durationMs = durationFor(m_phase);
    emit changed();
}
void TimerEngine::prepare(Phase phase)
{
    m_refresh.stop();
    m_checkpoint.stop();
    m_phase = phase;
    m_nextPhase = phase == Focus ? ShortBreak : Focus;
    m_state = Ready;
    m_durationMs = durationFor(phase);
    m_elapsedMs = 0;
    m_pauseReason.clear();
}
void TimerEngine::start()
{
    if (m_state == Running) return;
    if (m_state == Finished) prepare(m_nextPhase);
    // Keep the completed round visible until a new focus session actually starts.
    // Resuming a paused session must preserve the round it belongs to.
    if (m_state == Ready && m_phase == Focus && m_completedInCycle >= m_settings.cycles)
        m_completedInCycle = 0;
    m_startedAt = m_clock();
    m_state = Running;
    m_pauseReason.clear();
    m_refresh.start();
    m_checkpoint.start();
    emit changed();
    emit checkpointRequested();
}
void TimerEngine::pause(const QString &reason)
{
    if (m_state != Running) return;
    tick();
    if (m_state != Running) return;
    m_elapsedMs = elapsedMs();
    m_state = Paused;
    m_pauseReason = reason;
    m_refresh.stop();
    m_checkpoint.stop();
    emit changed();
    emit checkpointRequested();
}
void TimerEngine::reset()
{
    prepare(m_phase);
    emit changed();
    emit checkpointRequested();
}
void TimerEngine::selectPhase(int phase)
{
    if (phase < Focus || phase > LongBreak) return;
    prepare(static_cast<Phase>(phase));
    emit changed();
    emit checkpointRequested();
}
void TimerEngine::tick()
{
    if (m_state != Running) return;
    if (elapsedMs() >= m_durationMs) finish();
    else emit changed();
}
void TimerEngine::finish()
{
    m_elapsedMs = m_durationMs;
    m_state = Finished;
    m_refresh.stop();
    m_checkpoint.stop();
    if (m_phase == Focus) {
        m_completedInCycle = std::min(m_completedInCycle + 1, m_settings.cycles);
        m_nextPhase = m_completedInCycle >= m_settings.cycles ? LongBreak : ShortBreak;
    } else {
        m_nextPhase = Focus;
    }
    emit changed();
    emit stageCompleted(m_phase, m_durationMs, m_dateClock());
    emit checkpointRequested();
}
QJsonObject TimerEngine::snapshot() const
{
    // Account for a deadline crossed before the next event-loop tick.
    const qint64 storedElapsed = m_state == Running
        ? std::min(elapsedMs(), m_durationMs - 1) : m_elapsedMs;
    return {{"phase", m_phase}, {"state", m_state}, {"durationMs", m_durationMs},
            {"elapsedMs", storedElapsed}, {"completedInCycle", m_completedInCycle},
            {"nextPhase", m_nextPhase}};
}
bool TimerEngine::restore(const QJsonObject &json, QString &error)
{
    int phaseValue = 0, stateValue = 0, nextValue = 0, cycleValue = 0;
    qint64 duration = 0, elapsed = 0;
    if (!readInt(json, "phase", Focus, LongBreak, phaseValue)
        || !readInt(json, "state", Ready, Finished, stateValue)
        || !readInt(json, "nextPhase", Focus, LongBreak, nextValue)
        || !readInt(json, "completedInCycle", 0, 12, cycleValue)
        || !readMs(json, "durationMs", 60000, 180 * 60000, duration)
        || !readMs(json, "elapsedMs", 0, duration, elapsed)
        || (stateValue == Ready && elapsed != 0)
        || (stateValue == Finished && elapsed != duration)
        || ((stateValue == Running || stateValue == Paused) && elapsed >= duration)) {
        error = QStringLiteral("保存的计时进度无效，原文件已保留。");
        return false;
    }
    m_refresh.stop();
    m_checkpoint.stop();
    m_phase = static_cast<Phase>(phaseValue);
    m_state = stateValue == Running ? Paused : static_cast<State>(stateValue);
    m_nextPhase = static_cast<Phase>(nextValue);
    m_durationMs = duration;
    m_elapsedMs = elapsed;
    m_completedInCycle = cycleValue;
    m_pauseReason = m_state == Paused ? QStringLiteral("进度已恢复，点击继续开始计时") : QString();
    emit changed();
    return true;
}

