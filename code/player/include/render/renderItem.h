//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <QQuickFramebufferObject>
#include <QOpenGLFunctions>

class RenderItem : public QQuickFramebufferObject {
    Q_OBJECT
public:
    Renderer* createRenderer() const override;
};


class OpenGLRender : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions {
public:
    OpenGLRender();
    ~OpenGLRender();

    void render() override;
    void synchronize(QQuickFramebufferObject *item) override;
};
