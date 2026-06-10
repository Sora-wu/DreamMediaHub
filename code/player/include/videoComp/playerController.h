//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <QObject>
#include <QTimer>

namespace Dream {
    class DreamPlayer;
}

class AudioBufferDevice;
class QAudioSink;

class PlayerController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString mediaName READ mediaName NOTIFY mediaNameChanged)
    Q_PROPERTY(QString progressText READ progressText NOTIFY progressTextChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(uint32_t volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(uint32_t duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(uint32_t currentProgress READ currentProgress WRITE setCurrentProgress NOTIFY currentProgressChanged)

public:
    explicit PlayerController(QObject* parent = nullptr);
    ~PlayerController();

    [[nodiscard]] Dream::DreamPlayer* getPlayer() const {
        return player_;
    }

    QString mediaName() const;
    QString progressText() const;
    bool isPlaying() const;
    uint32_t volume() const;
    void setVolume(uint32_t newVolume);
    uint32_t duration() const;
    uint32_t currentProgress() const;

    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void togglePlay();
    Q_INVOKABLE void seek(double targetPos, double seekRel = 0);

    Q_INVOKABLE void setCurrentProgress(uint32_t newCurrentProgress);

signals:
    void mediaNameChanged();
    void progressTextChanged();
    void isPlayingChanged();
    void volumeChanged();
    void durationChanged();
    void currentProgressChanged();

private slots:
    void onTimerUpdate();

private:
    QString mediaName_{};
    QString progressText_{};
    bool isPlaying_ = false;

    Dream::DreamPlayer* player_ = nullptr;
    AudioBufferDevice* audioBufferDevice_ = nullptr;
    QAudioSink* audioSink_ = nullptr;
    uint32_t currentProgress_ = 0;
    uint32_t duration_ = 0;

    QTimer* timer_ = nullptr;
};
