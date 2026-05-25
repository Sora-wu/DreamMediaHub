//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <QQuickFramebufferObject>
#include <QOpenGLFunctions_4_5_Core>

#include <DreamPlayer/DreamContext.h>

class RenderItem : public QQuickFramebufferObject {
    Q_OBJECT
public:
    [[nodiscard]] Renderer* createRenderer() const override;
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
    void initTexture(GLuint textureHandle);
    void updateTexture(GLuint textureHandle, const char* uniformName, uint32_t textureIndex, uint32_t frameWidth, uint32_t frameHeight, int stride, char* data);

private:
    GLuint program_{};
    GLuint vao_{};
    GLuint vbo_{};
    GLuint ebo_{};
    GLuint texture_[3]{};
    int texWidth_[3]{};
    int texHeight_[3]{};

    bool initialized_ = false;

    Dream::DreamPlayer* player_ = nullptr;
    Dream::VideoFrame videoFrame_{};

    uint32_t itemWidth = 0;
    uint32_t itemHeight = 0;

    QOpenGLDebugLogger* logger_ = nullptr;
};
