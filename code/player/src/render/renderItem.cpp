//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <render/renderItem.h>

#include <DreamPlayer/DreamContext.h>

#include <QFile>
#include <QQuickOpenGLUtils>

namespace {
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

        return content.toStdString();
    }
}

using namespace Dream;

//////////////////////////RenderItem/////////////////////////////////
QQuickFramebufferObject::Renderer* RenderItem::createRenderer() const {
    return new OpenGLRender;
}

//////////////////////////OpenGLRender/////////////////////////////////
OpenGLRender::OpenGLRender() {
    player_ = DreamContext::createDreamPlayer();
}

OpenGLRender::~OpenGLRender() {
    cleanup();
}

void OpenGLRender::render() {
    if (!initialized_) {
        initialize();
        initialized_ = true;
    }

    glClearColor(0.0f, 0.0f, 0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program_);

    glBindVertexArray(vao_);

    VideoFrame videoFrame = player_->getVideoFrame();
    for (uint32_t i = 0; i < 3; ++i) {
        uint32_t textureHandle = texture_[i];
        const char* uniformName = textureUniformName[i];
        updateTexture(textureHandle, uniformName, i, videoFrame.frameWidth, videoFrame.frameHeight, videoFrame.data[i]);
    }

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
    glUseProgram(0);

    QQuickOpenGLUtils::resetOpenGLState();
    update();
}

void OpenGLRender::synchronize(QQuickFramebufferObject* item) {

}

bool OpenGLRender::compileShader() {
    // 顶点着色器
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    std::string vertexShaderStr = readShaderFile("shader/video.vert");
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
    std::string fragmentShaderStr = readShaderFile("shader/video.frag");
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

void OpenGLRender::initialize() {
    initializeOpenGLFunctions();

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

    const unsigned int indices[] = {
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
        initTexture(texture_[i]);
    }
}

void OpenGLRender::initTexture(GLuint textureHandle) {
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glTexParameteri(textureHandle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(textureHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(textureHandle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(textureHandle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLRender::updateTexture(GLuint textureHandle, const char* uniformName, uint32_t textureIndex, uint32_t frameWidth, uint32_t frameHeight, char* data) {
    glActiveTexture(GL_TEXTURE0 + textureIndex);
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glUniform1i(glGetUniformLocation(program_, uniformName), 0);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, (int)frameWidth / 2, (int)frameHeight / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}
