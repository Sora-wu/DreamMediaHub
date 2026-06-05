//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <videoComp/playerController.h>

#include <QQuickFramebufferObject>
#include <QOpenGLFunctions_4_5_Core>

namespace Dream {
    class DreamPlayer;
}

class OpenGLRender;

class VideoArea : public QQuickFramebufferObject {
    Q_OBJECT

    Q_PROPERTY(PlayerController* controller READ controller WRITE setController NOTIFY controllerChanged)

public:
    [[nodiscard]] Renderer* createRenderer() const override;
    PlayerController *controller() const;
    void setController(PlayerController *newController);

signals:
    void controllerChanged();

private:
    PlayerController *controller_ = nullptr;
};

class QOpenGLDebugLogger;

class OpenGLRender : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions_4_5_Core {
public:
    OpenGLRender();
    ~OpenGLRender() override;

    void render() override;
    void synchronize(QQuickFramebufferObject *item) override;

private:
    bool compileShader();
    void cleanup();
    void initDebugMessage();
    void initialize();
    void initTexture(GLuint textureHandle, const char* uniformName, uint32_t textureIndex);

    QRect updateItemRect() const;
    void updateMVP();
    void updateTexture(GLuint textureHandle, uint32_t textureIndex, uint32_t frameWidth, uint32_t frameHeight, int stride, char* data);

private:
    GLuint program_{};
    GLuint vao_{};
    GLuint vbo_{};
    GLuint ebo_{};
    GLuint texture_[3]{};
    GLint texUniformLocation_[3]{};
    GLint modelViewMatUniformLoc_{};
    int texWidth_[3]{};
    int texHeight_[3]{};

    bool initialized_ = false;

    float itemWidth = 0;
    float itemHeight = 0;

    QOpenGLDebugLogger* logger_ = nullptr;

    Dream::DreamPlayer* player_ = nullptr;
};
