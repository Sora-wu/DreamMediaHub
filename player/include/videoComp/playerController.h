//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <QObject>

namespace Dream {
    class DreamPlayer;
}

class AudioBufferDevice;
class QAudioSink;
class QTimer;

class PlayerController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString mediaName READ mediaName NOTIFY mediaNameChanged)
    Q_PROPERTY(QString progressText READ progressText NOTIFY progressTextChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(uint32_t volume READ volume NOTIFY volumeChanged)
    Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(uint32_t duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(uint32_t currentProgress READ currentProgress WRITE setCurrentProgress NOTIFY currentProgressChanged)

public:
    explicit PlayerController(QObject* parent = nullptr);
    ~PlayerController() override = default;

    [[nodiscard]] Dream::DreamPlayer* getPlayer() const {
        return player_;
    }

    QString mediaName() const;
    QString progressText() const;
    bool isPlaying() const;
    uint32_t volume() const;
    uint32_t duration() const;
    double speed() const;
    void setSpeed(double speed);
    uint32_t currentProgress() const;
    void setCurrentProgress(uint32_t newCurrentProgress);

    Q_INVOKABLE void open(const QString& path);
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void togglePlay();
    // isRelative是否为相对时间
    Q_INVOKABLE void seek(int deltaSeconds, bool isRelative = true);
    Q_INVOKABLE void setVolume(uint32_t newVolume);

    Q_INVOKABLE void cleanup();

signals:
    void mediaNameChanged();
    void progressTextChanged();
    void isPlayingChanged();
    void volumeChanged();
    void durationChanged();
    void speedChanged();
    void currentProgressChanged();

private slots:
    void onTimerUpdate();
    void onPlaybackCompleted();

private:
    QString mediaName_{};
    QString progressText_{};
    uint32_t volume_ = 100;
    bool isPlaying_ = false;

    Dream::DreamPlayer* player_ = nullptr;
    AudioBufferDevice* audioBufferDevice_ = nullptr;
    QAudioSink* audioSink_ = nullptr;
    double speed_ = 1.0f;
    uint32_t currentProgress_ = 0;
    uint32_t duration_ = 0;

    double seekTarget_ = 0;
    bool seekActive_ = false;

    QTimer* timer_ = nullptr;
    QTimer* seekTimer_ = nullptr;
};
