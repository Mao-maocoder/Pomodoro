#pragma once
#include <QAudioDevice>
#include <QAudioSink>
#include <QBuffer>
#include <QElapsedTimer>
#include <QMediaDevices>
#include <QObject>
#include <QTimer>
#include <functional>
#include <memory>
#include <vector>

// Each request owns a fresh output stream; no old voice can migrate to a new device.
class ChimePlayer final : public QObject {
    Q_OBJECT
public:
    using DeviceProvider = std::function<QAudioDevice()>;
    explicit ChimePlayer(QString diagnosticPath, QObject *parent = nullptr,
                         DeviceProvider deviceProvider = {});
    ~ChimePlayer() override;
    void play(int volume, const QString &reason);
    void stop(const QString &reason = QStringLiteral("cancelled"));
    bool isPlaying() const { return m_active; }
    float volume() const { return m_volume / 100.0f; }
    QString errorString() const;
    qint64 lastProcessedUSecs() const { return m_lastProcessed; }
    int attempts() const { return m_attempt; }
    quint64 requestId() const { return m_requestId; }
signals:
    void playingChanged();
    void errorChanged();
    void playbackStarted(quint64 requestId);
    void playbackFinished(quint64 requestId);
    void playbackFailed(quint64 requestId, const QString &error);
private slots:
    void audioOutputsChanged();
private:
    bool loadWave();
    QByteArray render(const QAudioFormat &format) const;
    void beginAttempt();
    void poll();
    void handleState(QtAudio::State state);
    void retryOrFail(const QString &error);
    void fail(const QString &error);
    void finish();
    void releaseOutput();
    void setError(QString error);
    void log(const QString &event, const QString &detail = {});
    QString m_diagnosticPath;
    DeviceProvider m_deviceProvider;
    QMediaDevices m_devices;
    std::unique_ptr<QAudioSink> m_sink;
    QBuffer m_buffer;
    QTimer m_watchdog;
    QTimer m_retryTimer;
    QElapsedTimer m_requestClock;
    std::vector<float> m_samples;
    int m_sampleRate = 0;
    QString m_resourceError;
    QString m_error;
    QString m_diagnosticError;
    QString m_reason;
    QString m_deviceName;
    int m_volume = 0;
    int m_attempt = 0;
    quint64 m_requestId = 0;
    qint64 m_attemptStarted = 0;
    qint64 m_expectedDuration = 0;
    qint64 m_lastProcessed = 0;
    bool m_active = false;
    bool m_started = false;
};