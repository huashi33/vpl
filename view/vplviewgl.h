#ifndef VPLVIEWGL_H
#define VPLVIEWGL_H


#include "src/vplrqframe.hpp"
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>

struct SwsContext;

class VplViewGL : public QOpenGLWidget,public QOpenGLFunctions_3_3_Core,public vpl::RingQueueAvFrameHandler{
    Q_OBJECT

public:
    explicit VplViewGL(QWidget *parent = nullptr);
    ~VplViewGL();
    void onHandleStart(AVFrame *frameSrc);
    void onHandleStop();
    void onHandle(AVFrame *frameSrc);

protected:
    void initializeGL();
    void resizeGL(int width, int height);
    void paintGL();
signals:
    void imgUpdated(AVFrame *);
public slots:
    void onImgUpdated(AVFrame *);


private:
    //return mProgram
    int readAllFromFile(char **buf,const char *filename);
    GLuint initShaderFromFile(const char *vs,const char *fs);
private:
    GLuint vao,vbo;
    GLuint mProgram;
    GLuint mTexture;
    AVFrame *frameRgb;
    SwsContext *img_convert_ctx;
};

#endif // VPLVIEW_H
