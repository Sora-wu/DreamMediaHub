//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <videoComp/videoArea.h>
#include <videoComp/videoRenderAccess.h>

#include <QMatrix4x4>
#include <QFile>
#include <QQuickOpenGLUtils>
#include <QOpenGLDebugLogger>

#include <DreamPlayer/DreamPlayer.h>

namespace {
    const double EPSILON = 1e-8;
    const char* textureUniformName[3] = { "textureY", "textureU", "textureV" };

    std::string readShaderFile(const QString& filePath) {
        QFile file(filePath);

        // 尝试以只读和文本模式打开文件
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << QString("error: cannot open file: %1 error info: %2").arg(filePath, file.errorString());
            return {};
        }

        QTextStream in(&file);
        const QString content = in.readAll();
        file.close();

        // GLSL 版本由公共平台定义提供，资源文件只保留着色器主体。
        return std::string(PLAYER_GLSL_VERSION) + content.toStdString();
    }
}

using namespace Dream;

//////////////////////////VideoArea/////////////////////////////////
QQuickFramebufferObject::Renderer* VideoArea::createRenderer() const {
    return new OpenGLRender;
}


PlayerController *VideoArea::controller() const
{
    return controller_.data();
}

void VideoArea::setController(PlayerController *newController)
{
    if (controller_.data() == newController)
        return;

    QObject::disconnect(videoSourceConnection_);
    QObject::disconnect(controllerDestroyedConnection_);
    controller_ = newController;
    if (newController) {
        videoSourceConnection_ = connect(newController, &PlayerController::videoSourceChanged, this, [this] {
            update();
        });

        controllerDestroyedConnection_ = connect(newController, &QObject::destroyed, this, [this] {
            update();
            emit controllerChanged();
        });
    }
    update();
    emit controllerChanged();
}

//////////////////////////OpenGLRender/////////////////////////////////
OpenGLRender::OpenGLRender() {}

OpenGLRender::~OpenGLRender() {
    cleanup();
}

void OpenGLRender::render() {
    if (!initialized_) {
        initialize();
        initialized_ = true;
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const bool active = renderAccess_ && renderAccess_->withPlayer([this](DreamPlayer& player) {
        VideoFrame* videoFrame = player.getVideoFrame();
        if (!videoFrame || !videoFrame->data[0] || !videoFrame->frameWidth || !videoFrame->frameHeight) {
            return;
        }

        glUseProgram(program_);
        updateMVP(videoFrame);

        for (uint32_t i = 0; i < 3; ++i) {
            updateTexture(texture_[i], i, videoFrame->frameWidth, videoFrame->frameHeight,
                          videoFrame->stride[i], videoFrame->data[i]);
        }

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
        glUseProgram(0);
    });

    QQuickOpenGLUtils::resetOpenGLState();
    if (active) {
        update();
    }
}

void OpenGLRender::synchronize(QQuickFramebufferObject* item) {
    VideoArea* area = static_cast<VideoArea*>(item);

    itemWidth = area->width();
    itemHeight = area->height();

    PlayerController* controller = area->controller();
    renderAccess_ = controller ? controller->renderAccess() : nullptr;
}

bool OpenGLRender::compileShader() {
    // 顶点着色器
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    std::string vertexShaderStr = readShaderFile(":/qt/qml/Dream/play/resource/shader/video.vert");
    const char* vertexShaderSource = vertexShaderStr.c_str();
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512]{};
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        qWarning() << "vertex shader compile fail:" << infoLog;
        return false;
    }

    // 片段着色器
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    std::string fragmentShaderStr = readShaderFile(":/qt/qml/Dream/play/resource/shader/video.frag");
    const char* fragmentShaderSource = fragmentShaderStr.c_str();
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        qWarning() << "fragment shader compile fail:" << infoLog;
        return false;
    }

    // 着色器程序
    program_ = glCreateProgram();
    glAttachShader(program_, vertexShader);
    glAttachShader(program_, fragmentShader);
    glLinkProgram(program_);

    glGetProgramiv(program_, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(program_, 512, nullptr, infoLog);
        qWarning() << "shader link compile fail:" << infoLog;
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
}

void OpenGLRender::cleanup() {
    glDeleteProgram(program_);
    program_ = 0;

    glDeleteVertexArrays(1, &vao_);
    vao_ = 0;

    glDeleteBuffers(1, &vbo_);
    vbo_ = 0;

    glDeleteBuffers(1, &ebo_);
    ebo_ = 0;

    glDeleteTextures(3, texture_);
}

void OpenGLRender::initDebugMessage() {
    logger_ = new QOpenGLDebugLogger;
    if (logger_->initialize()) {
        logger_->disableMessages(QOpenGLDebugMessage::AnySource, QOpenGLDebugMessage::AnyType,QOpenGLDebugMessage::NotificationSeverity);
        QObject::connect(logger_, &QOpenGLDebugLogger::messageLogged, [](const QOpenGLDebugMessage& msg){
            qWarning() << msg;
        });
        logger_->startLogging();
    }
}

void OpenGLRender::initialize() {
    initializeOpenGLFunctions();

    initDebugMessage();

    if (!compileShader()) {
        return;
    }

    const float vertices[] = {
        // 位置              // 纹理坐标
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,  // 左下
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,  // 右下
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f,  // 右上
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f   // 左上
    };

    const uint32_t indices[] = {
        0, 1, 2,  // 第一个三角形
        2, 3, 0   // 第二个三角形
    };

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenTextures(3, texture_);
    for (uint32_t i = 0; i < 3; ++i) {
        initTexture(texture_[i], textureUniformName[i], i);
    }

    modelViewMatUniformLoc_ = glGetUniformLocation(program_, "modelViewMatrix");
}

void OpenGLRender::initTexture(GLuint textureHandle, const char* uniformName, uint32_t textureIndex) {
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    texUniformLocation_[textureIndex] = glGetUniformLocation(program_, uniformName);
    glBindTexture(GL_TEXTURE_2D, 0);
}

QRect OpenGLRender::updateItemRect(VideoFrame* videoFrame) const {
    const uint32_t height = videoFrame->frameHeight;
    const uint32_t width = videoFrame->frameWidth;

    double aspectRatio = (double)width / height;
    if (videoFrame->picWidthHeightRatio > EPSILON) {
        aspectRatio *= videoFrame->picWidthHeightRatio;
    }

    uint32_t rectHeight = itemHeight;
    uint32_t rectWidth = (uint32_t)std::round((float)rectHeight * aspectRatio) & ~1;
    if (rectWidth > itemWidth) {
        rectWidth = itemWidth;
        rectHeight = (uint32_t)std::round((float)rectWidth / aspectRatio) & ~1;
    }

    QRect itemRect{};
    itemRect.setX((itemWidth - rectWidth) / 2);
    itemRect.setY((itemHeight - rectHeight) / 2);
    itemRect.setWidth(rectWidth);
    itemRect.setHeight(rectHeight);

    return itemRect;
}

void OpenGLRender::updateMVP(VideoFrame* videoFrame) {
    const QRect itemRect = updateItemRect(videoFrame);
    const float left = itemRect.left();
    const float right = left + itemRect.width();
    const float top = itemRect.top();
    const float bottom = top + itemRect.height();

    // 这里需要上下颠倒，OpenGL的uv是倒过来的
    const float vertices[] = {
        // 位置                     // 纹理坐标
        left, bottom, 0.0f,        0.0f, 1.0f,  // 左下
        right, bottom, 0.0f,       1.0f, 1.0f,  // 右下
        right, top, 0.0f,          1.0f, 0.0f,  // 右上
        left, top, 0.0f,           0.0f, 0.0f   // 左上
    };

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(vao_);

    QMatrix4x4 modelViewMatrix{};
    modelViewMatrix.ortho(0, itemWidth, 0, itemHeight, -1, 1);

    glUniformMatrix4fv(modelViewMatUniformLoc_, 1, GL_FALSE, modelViewMatrix.constData());
}

void OpenGLRender::updateTexture(GLuint textureHandle, uint32_t textureIndex, uint32_t frameWidth, uint32_t frameHeight, int stride, char* data) {
    if (!data || !frameWidth || !frameHeight || stride <= 0) {
        return;
    }

    const int width = (textureIndex == 0) ? static_cast<int>(frameWidth) : static_cast<int>((frameWidth + 1) / 2);
    const int height = (textureIndex == 0) ? static_cast<int>(frameHeight) : static_cast<int>((frameHeight + 1) / 2);
    if (width <= 0 || height <= 0) {
        return;
    }

    glActiveTexture(GL_TEXTURE0 + textureIndex);
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glUniform1i(texUniformLocation_[textureIndex], (int)textureIndex);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, std::max(stride, width));
    if (width != texWidth_[textureIndex] || height != texHeight_[textureIndex]) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
        texWidth_[textureIndex] = width;
        texHeight_[textureIndex] = height;
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, data);
    }
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}
