#include "ChimePlayer.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtEndian>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

namespace {
constexpr qint64 startDeadlineMs = 3000;
constexpr qint64 requestDeadlineMs = 6500;
constexpr qint64 progressDeadlineMs = 800;
}
ChimePlayer::ChimePlayer(QString diagnosticPath, QObject *parent, DeviceProvider deviceProvider)
    : QObject(parent), m_diagnosticPath(std::move(diagnosticPath)),
      m_deviceProvider(std::move(deviceProvider)), m_devices(this), m_buffer(this)
{
    if (!m_deviceProvider)
        m_deviceProvider = [] { return QMediaDevices::defaultAudioOutput(); };
    loadWave();
    m_watchdog.setInterval(100);
    connect(&m_watchdog, &QTimer::timeout, this, &ChimePlayer::poll);
    m_retryTimer.setSingleShot(true);
    m_retryTimer.setInterval(200);
    connect(&m_retryTimer, &QTimer::timeout, this, &ChimePlayer::beginAttempt);
    connect(&m_devices, &QMediaDevices::audioOutputsChanged,
            this, &ChimePlayer::audioOutputsChanged);
}
ChimePlayer::~ChimePlayer() { releaseOutput(); }
QString ChimePlayer::errorString() const
{
    if (m_diagnosticError.isEmpty()) return m_error;
    return m_error.isEmpty() ? m_diagnosticError : m_error + "\n" + m_diagnosticError;
}
bool ChimePlayer::loadWave()
{
    QFile file(QStringLiteral(":/assets/chime.wav"));
    if (!file.open(QIODevice::ReadOnly)) {
        m_resourceError = QStringLiteral("无法读取提示音：%1").arg(file.errorString());
        return false;
    }
    const QByteArray wave = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        m_resourceError = QStringLiteral("提示音读取失败：%1").arg(file.errorString());
        return false;
    }
    auto invalid = [this] {
        m_resourceError = QStringLiteral("提示音格式无效，需要单声道 16 位 PCM WAV。");
        return false;
    };
    if (wave.size() < 12 || wave.first(4) != "RIFF" || wave.mid(8, 4) != "WAVE")
        return invalid();
    QByteArray pcm;
    bool validFormat = false;
    for (qsizetype offset = 12; offset + 8 <= wave.size();) {
        const quint32 size = qFromLittleEndian<quint32>(wave.constData() + offset + 4);
        if (quint64(size) > quint64(wave.size() - offset - 8)) return invalid();
        const char *chunk = wave.constData() + offset + 8;
        if (wave.mid(offset, 4) == "fmt ") {
            if (size < 16) return invalid();
            validFormat = qFromLittleEndian<quint16>(chunk) == 1
                && qFromLittleEndian<quint16>(chunk + 2) == 1
                && qFromLittleEndian<quint16>(chunk + 12) == 2
                && qFromLittleEndian<quint16>(chunk + 14) == 16;
            m_sampleRate = int(qFromLittleEndian<quint32>(chunk + 4));
        } else if (wave.mid(offset, 4) == "data") {
            pcm = QByteArray(chunk, size);
        }
        offset += 8 + qsizetype(size) + (size & 1);
    }
    if (!validFormat || m_sampleRate < 8000 || m_sampleRate > 192000
        || pcm.isEmpty() || pcm.size() % 2 != 0) return invalid();
    m_samples.resize(size_t(pcm.size() / 2));
    for (size_t i = 0; i < m_samples.size(); ++i)
        m_samples[i] = qFromLittleEndian<qint16>(pcm.constData() + i * 2) / 32768.0f;
    return true;
}
QByteArray ChimePlayer::render(const QAudioFormat &format) const
{
    const qint64 frames = qint64(m_samples.size()) * format.sampleRate() / m_sampleRate;
    QByteArray output(frames * format.bytesPerFrame(), Qt::Uninitialized);
    int left = format.channelOffset(QAudioFormat::FrontLeft);
    int right = format.channelOffset(QAudioFormat::FrontRight);
    if (format.channelConfig() == QAudioFormat::ChannelConfigUnknown) {
        left = 0;
        right = format.channelCount() > 1 ? 1 : 0;
    }
    char *destination = output.data();
    for (qint64 frame = 0; frame < frames; ++frame) {
        const double position = double(frame) * m_sampleRate / format.sampleRate();
        const size_t a = std::min(size_t(position), m_samples.size() - 1);
        const size_t b = std::min(a + 1, m_samples.size() - 1);
        const float sample = m_samples[a] + float(position - a) * (m_samples[b] - m_samples[a]);
        for (int channel = 0; channel < format.channelCount(); ++channel) {
            const float value = format.channelCount() == 1 || channel == left || channel == right
                ? sample : 0.0f;
            switch (format.sampleFormat()) {
            case QAudioFormat::UInt8: {
                const quint8 encoded = quint8(std::clamp(std::lround(value * 127.f + 128.f), 0L, 255L));
                std::memcpy(destination, &encoded, sizeof(encoded));
                break;
            }
            case QAudioFormat::Int16: {
                const qint16 encoded = qint16(std::clamp(std::lround(value * 32767.f), -32768L, 32767L));
                std::memcpy(destination, &encoded, sizeof(encoded));
                break;
            }
            case QAudioFormat::Int32: {
                const qint32 encoded = qint32(std::llround(double(value) * 2147483647.0));
                std::memcpy(destination, &encoded, sizeof(encoded));
                break;
            }
            case QAudioFormat::Float:
                std::memcpy(destination, &value, sizeof(value));
                break;
            default:
                Q_UNREACHABLE();
            }
            destination += format.bytesPerSample();
        }
    }
    return output;
}
void ChimePlayer::play(int volume, const QString &reason)
{
    stop(QStringLiteral("replaced"));
    ++m_requestId;
    m_volume = std::clamp(volume, 0, 100);
    m_reason = reason;
    m_deviceName.clear();
    m_attempt = 0;
    m_lastProcessed = 0;
    m_started = false;
    setError({});
    m_requestClock.start();
    if (m_volume == 0) {
        log(QStringLiteral("muted"));
        return;
    }
    m_active = true;
    emit playingChanged();
    log(QStringLiteral("request"));
    if (!m_resourceError.isEmpty()) {
        fail(m_resourceError);
        return;
    }
    m_watchdog.start();
    beginAttempt();
}
void ChimePlayer::beginAttempt()
{
    if (!m_active) return;
    if (m_requestClock.elapsed() >= startDeadlineMs) {
        fail(QStringLiteral("提示音启动已超时，旧请求已取消。"));
        return;
    }
    releaseOutput();
    ++m_attempt;
    m_attemptStarted = m_requestClock.elapsed();
    const QAudioDevice device = m_deviceProvider();
    m_deviceName = device.description();
    log(QStringLiteral("attempt"));
    if (device.isNull()) {
        retryOrFail(QStringLiteral("没有可用的声音输出设备。"));
        return;
    }
    const QAudioFormat format = device.preferredFormat();
    if (!format.isValid() || format.sampleRate() < 8000 || format.sampleRate() > 192000
        || format.channelCount() > 8 || !device.isFormatSupported(format)
        || format.sampleFormat() == QAudioFormat::Unknown) {
        retryOrFail(QStringLiteral("当前音频设备的输出格式不受支持：%1").arg(m_deviceName));
        return;
    }
    m_buffer.setData(render(format));
    if (!m_buffer.open(QIODevice::ReadOnly)) {
        fail(QStringLiteral("无法准备提示音播放缓冲区：%1").arg(m_buffer.errorString()));
        return;
    }
    m_expectedDuration = format.durationForBytes(qint32(m_buffer.size()));
    m_sink = std::make_unique<QAudioSink>(device, format);
    m_sink->setVolume(m_volume / 100.0);
    const quint64 request = m_requestId;
    const int attempt = m_attempt;
    connect(m_sink.get(), &QAudioSink::stateChanged, this,
            [this, request, attempt](QtAudio::State state) {
        if (m_active && request == m_requestId && attempt == m_attempt)
            handleState(state);
    }, Qt::QueuedConnection);
    m_sink->start(&m_buffer);
    if (m_sink->error() != QtAudio::NoError) {
        retryOrFail(QStringLiteral("音频输出启动失败，错误码 %1，设备：%2")
                    .arg(int(m_sink->error())).arg(m_deviceName));
    }
}
void ChimePlayer::poll()
{
    if (!m_active) return;
    if (m_requestClock.elapsed() >= requestDeadlineMs) {
        fail(QStringLiteral("提示音播放超时，旧请求已取消；请检查音频设备。"));
        return;
    }
    if (!m_sink) return;
    m_lastProcessed = std::max(m_lastProcessed, m_sink->processedUSecs());
    if (!m_started && m_lastProcessed > 0) {
        m_started = true;
        log(QStringLiteral("started"));
        emit playbackStarted(m_requestId);
    }
    if (m_sink->error() != QtAudio::NoError) {
        retryOrFail(QStringLiteral("音频输出发生错误 %1，设备：%2")
                    .arg(int(m_sink->error())).arg(m_deviceName));
        return;
    }
    if (m_sink->state() == QtAudio::IdleState && m_buffer.atEnd()) {
        // Some backends count frames queued to the device; allow one short buffer of rounding.
        if (m_lastProcessed + 100000 >= m_expectedDuration) finish();
        else retryOrFail(QStringLiteral("提示音提前停止，音频数据未完整输出。"));
        return;
    }
    if (!m_started && m_requestClock.elapsed() - m_attemptStarted >= progressDeadlineMs)
        retryOrFail(QStringLiteral("音频输出未开始处理提示音。"));
}
void ChimePlayer::handleState(QtAudio::State state)
{
    if (!m_sink) return;
    log(QStringLiteral("state"), QString::number(int(state)));
    if (state == QtAudio::StoppedState) {
        retryOrFail(QStringLiteral("提示音输出已停止，错误码 %1，设备：%2")
                    .arg(int(m_sink->error())).arg(m_deviceName));
    } else {
        poll();
    }
}
void ChimePlayer::retryOrFail(const QString &error)
{
    if (!m_active || m_retryTimer.isActive()) return;
    if (m_sink)
        m_lastProcessed = std::max(m_lastProcessed, m_sink->processedUSecs());
    log(QStringLiteral("attempt-failed"), error);
    releaseOutput();
    if (m_lastProcessed == 0 && m_attempt < 2 && m_requestClock.elapsed() < startDeadlineMs) {
        m_retryTimer.start();
    } else {
        fail(error);
    }
}
void ChimePlayer::finish()
{
    log(QStringLiteral("finished"));
    stop(QStringLiteral("completed"));
    setError({});
    emit playbackFinished(m_requestId);
}
void ChimePlayer::fail(const QString &error)
{
    log(QStringLiteral("failed"), error);
    stop(QStringLiteral("failed"));
    setError(error + QStringLiteral(" 到点的完成提示仍然有效，连接设备后请重新点击试听。"));
    emit playbackFailed(m_requestId, m_error);
}
void ChimePlayer::stop(const QString &reason)
{
    m_watchdog.stop();
    m_retryTimer.stop();
    const bool wasActive = std::exchange(m_active, false);
    releaseOutput();
    if (wasActive) {
        log(QStringLiteral("cancelled"), reason);
        emit playingChanged();
    }
}
void ChimePlayer::releaseOutput()
{
    if (m_sink) {
        disconnect(m_sink.get(), nullptr, this, nullptr);
        m_sink->reset();
        m_sink.reset();
    }
    m_buffer.close();
}
void ChimePlayer::audioOutputsChanged()
{
    log(QStringLiteral("devices-changed"));
    if (m_active) {
        fail(QStringLiteral("音频设备发生变化，已取消旧铃声以避免延迟补响。"));
    }
}
void ChimePlayer::setError(QString error)
{
    if (m_error == error) return;
    m_error = std::move(error);
    emit errorChanged();
}
void ChimePlayer::log(const QString &event, const QString &detail)
{
    const QJsonObject entry{
        {"time", QDateTime::currentDateTime().toString(Qt::ISODateWithMs)},
        {"event", event}, {"request", qint64(m_requestId)}, {"reason", m_reason},
        {"attempt", m_attempt}, {"device", m_deviceName}, {"volume", m_volume},
        {"elapsedMs", m_requestClock.isValid() ? m_requestClock.elapsed() : 0},
        {"processedUSecs", m_lastProcessed}, {"detail", detail}
    };
    const QByteArray line = QJsonDocument(entry).toJson(QJsonDocument::Compact) + '\n';
    QFile file(m_diagnosticPath);
    QString error;
    if (!QDir().mkpath(QFileInfo(m_diagnosticPath).absolutePath())) {
        error = QStringLiteral("无法创建铃声诊断目录。");
    } else if (!file.open(QIODevice::WriteOnly | QIODevice::Append)
               || file.write(line) != line.size() || !file.flush()) {
        error = QStringLiteral("无法写入铃声诊断记录：%1").arg(file.errorString());
    }
    if (m_diagnosticError != error) {
        m_diagnosticError = error;
        emit errorChanged();
    }
}