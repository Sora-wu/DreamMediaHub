//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <QQuickFramebufferObject>
#include <QOpenGLExtraFunctions>

#include <DreamPlayer/DreamContext.h>

class RenderItem : public QQuickFramebufferObject {
    Q_OBJECT
public:
    [[nodiscard]] Renderer* createRenderer() const override;
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
    void updateTexture(GLuint textureHandle, const char* uniformName, uint32_t textureIndex, uint32_t frameWidth, uint32_t frameHeight, char* data);

private:
    GLuint program_{};
    GLuint vao_{};
    GLuint vbo_{};
    GLuint ebo_{};
    GLuint texture_[3]{};

    bool initialized_ = false;

    Dream::DreamPlayer* player_ = nullptr;
    Dream::VideoFrame videoFrame_{};
};
