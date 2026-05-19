//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <QQuickFramebufferObject>
#include <QOpenGLExtraFunctions>

namespace Dream {
    class DreamPlayer;
    struct VideoFrame;
}

class RenderItem : public QQuickFramebufferObject {
    Q_OBJECT
public:
    Renderer* createRenderer() const override;
};


class OpenGLRender : public QQuickFramebufferObject::Renderer, protected QOpenGLExtraFunctions {
public:
    OpenGLRender();
    ~OpenGLRender() override;

    void render() override;
    void synchronize(QQuickFramebufferObject *item) override;

private:
    bool compileShader();
    void cleanup();
    void initialize();
    void initTexture(GLuint textureHandle);
    void updateTexture(GLuint textureHandle, const char* uniformName, uint32_t textureIndex, char* data, uint32_t len);

    void onVideoFrameRefresh(Dream::VideoFrame* videoFrame);

private:
    GLuint program_{};
    GLuint vao_{};
    GLuint vbo_{};
    GLuint ebo_{};
    GLuint textureY_{};
    GLuint textureU_{};
    GLuint textureV_{};

    bool initialized_ = false;

    Dream::DreamPlayer* player_ = nullptr;
};
