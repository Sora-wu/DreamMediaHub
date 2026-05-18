//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <QQuickFramebufferObject>
#include <QOpenGLExtraFunctions>

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

    // void onVideoFrameRefresh();

private:
    GLuint program_{};
    GLuint vao_{};
    GLuint vbo_{};
    GLuint ebo_{};
    GLuint textureY_{};
    GLuint textureU_{};
    GLuint textureV_{};

    bool initialized_ = false;

    // char* dataY_{};
    // uint32_t lenY_{};
    // char* dataU_{};
    // uint32_t lenU_{};
    // char* dataV_{};
    // uint32_t lenV_{};
};
