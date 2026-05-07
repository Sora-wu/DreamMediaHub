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
    bool complieShader();
    void cleanup();
    void initialize();

private:
    GLuint program_{};
    GLuint vao_{};
    GLuint vbo_{};
    GLuint ebo_{};

    bool initialized_ = false;
};
