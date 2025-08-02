#include "vplviewgl.h"
#include <thread>
#include <functional>
#include "src/vpldefine.h"

extern "C" {
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
    #include <libavutil/time.h>
}






VplViewGL::VplViewGL(QWidget *parent) :QOpenGLWidget(parent){
    frameRgb = nullptr;
    img_convert_ctx = nullptr;
    vao = 0;
    vbo = 0;
    mTexture = 0;
    mProgram = 0;

    connect(this,&VplViewGL::imgUpdated,this,&VplViewGL::onImgUpdated);
}

VplViewGL::~VplViewGL(){
    if (!mProgram) {
         return;
    }

    makeCurrent();
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    glDeleteVertexArrays(1,&vao);
    glDeleteBuffers(1,&vbo);
    glDeleteTextures(1,&mTexture);
    glDeleteProgram(mProgram);
    doneCurrent();
}

void VplViewGL::onHandleStart(AVFrame *frameSrc){
    frameRgb = av_frame_alloc();
    frameRgb->width = this->width();
    frameRgb->height = this->height();
    frameRgb->format = AV_PIX_FMT_RGB24;
    av_frame_get_buffer(frameRgb,0);
    img_convert_ctx = sws_getContext(frameSrc->width,frameSrc->height,(AVPixelFormat)frameSrc->format,
                                    frameRgb->width,frameRgb->height,(AVPixelFormat)frameRgb->format,
                                    SWS_BILINEAR, nullptr, nullptr, nullptr);
}

void VplViewGL::onHandleStop(){
    if(frameRgb){
        av_frame_free(&frameRgb);
        frameRgb = nullptr;
    }
    if(img_convert_ctx){
        sws_freeContext(img_convert_ctx);
        img_convert_ctx = nullptr;
    }
}

void VplViewGL::onHandle(AVFrame *frameSrc){
    int cvt = sws_scale(img_convert_ctx,frameSrc->data,frameSrc->linesize,0,frameSrc->height,frameRgb->data,frameRgb->linesize);
    // makeCurrent();


    // doneCurrent();
    emit imgUpdated(frameRgb);
    // update();
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
}

void VplViewGL::initializeGL(){
    initializeOpenGLFunctions();
    const float vertex[]={
        -1.0f,-1.0f,0.0f, 0.0f,1.0f,
        1.0f,-1.0f,0.0f,  1.0f,1.0f,
        1.0f,1.0f,0.0f,   1.0f,0.0f,
        -1.0f,1.0f,0.0f,  0.0f,0.0f
    };

    //init vao
    glGenVertexArrays(1,&vao);
    glBindVertexArray(vao);

    //init vbo
    glGenBuffers(1,&vbo);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof (vertex),vertex,GL_STATIC_DRAW);//UP TO GPU
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,5*sizeof (float),(void*)0);//how to use vbo
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,5*sizeof (float),(void*)(3*sizeof (float)));//how to use vbo
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    //shader
    mProgram = initShaderFromFile("rawv.glsl","rawf.glsl");

    //Texture
    glGenTextures(1,&mTexture);
    glBindTexture(GL_TEXTURE_2D,mTexture);
    // QImage img("1.jpg");
    // img = img.convertToFormat(QImage::Format_RGB888).mirrored();
    // glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,img.width(),img.height(),0,GL_RGB,GL_UNSIGNED_BYTE,img.bits());
    // glGenerateMipmap(GL_TEXTURE_2D);
}
void VplViewGL::resizeGL(int width, int height){
}
void VplViewGL::paintGL(){
    if(!frameRgb){
        return;
    }
    glBindTexture(GL_TEXTURE_2D,mTexture);
    // uint32_t w = frameRgb->width;
    uint32_t w = frameRgb->linesize[0]/3;
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,frameRgb->height,0,GL_RGB,GL_UNSIGNED_BYTE,frameRgb->data[0]);
    glGenerateMipmap(GL_TEXTURE_2D);

    glDrawArrays(GL_QUADS,0,4);
}

int VplViewGL::readAllFromFile(char **buf, const char *filename){
    *buf = nullptr;
    FILE * f = fopen(filename,"r");
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);
    if(0 < file_size){
        char *tmpbuf = (char *)calloc(1,file_size + 1);
        fread(tmpbuf,1,file_size,f);
        *buf = tmpbuf;
    }
    fclose(f);
    return file_size;
}

GLuint VplViewGL::initShaderFromFile(const char *vs, const char *fs){
    char *vsSrc = nullptr,*fsSrc = nullptr;
    if(!readAllFromFile(&vsSrc,vs) || !vsSrc){
        return 0;
    }
    if(!readAllFromFile(&fsSrc,fs) || !fsSrc){
        return 0;
    }


    int  success;
    char infoLog[512] = {0};
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader,1,&vsSrc,nullptr);
    glCompileShader(vShader);
    glGetShaderiv(vShader, GL_COMPILE_STATUS, &success);
    if(!success){
        glGetShaderInfoLog(vShader, 512, NULL, infoLog);
        printf("vshader compile fail:%s\n",infoLog);
        free(vsSrc);
        free(fsSrc);
        return 0;
    }
    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader,1,&fsSrc,nullptr);
    glCompileShader(fShader);
    glGetShaderiv(fShader, GL_COMPILE_STATUS, &success);
    if(!success){
        glGetShaderInfoLog(fShader, 512, NULL, infoLog);
        printf("fshader compile fail:%s\n",infoLog);
        free(vsSrc);
        free(fsSrc);
        return 0;
    }


    GLuint mp = glCreateProgram();
    glAttachShader(mp,vShader);
    glAttachShader(mp,fShader);
    glLinkProgram(mp);
    glGetProgramiv(mp, GL_COMPILE_STATUS, &success);
    if(!success){
        glGetProgramInfoLog(mp, 512, NULL, infoLog);
        printf("program compile fail:%s\n",infoLog);
        free(vsSrc);
        free(fsSrc);
        return 0;
    }

    glUseProgram(mp);
    glDeleteShader(vShader);
    glDeleteShader(fShader);

    free(vsSrc);
    free(fsSrc);
    return mp;
}

void VplViewGL::onImgUpdated(AVFrame *frame){
    // this->frame = frame;
    this->update();
}
