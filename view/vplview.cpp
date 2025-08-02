#include "vplview.h"
#include "ui_vplview.h"
#include <thread>
#include <functional>
#include "src/vpldefine.h"
#include "QPainter"

extern "C" {
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
    #include <libavutil/time.h>
}






VplView::VplView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VplView){
    ui->setupUi(this);

    connect(this,&VplView::imgUpdated,this,&VplView::onImgUpdated);
    
    frameRgb = nullptr;
    img_convert_ctx = nullptr;
}

VplView::~VplView(){
    delete ui;
}
bool VplView::start(){
    if(!vpl::RqsiPopFrameHandler::start()){
        return false;
    }
    frameRgb = av_frame_alloc();
    frameRgb->width = this->width();
    frameRgb->height = this->height();
    frameRgb->format = AV_PIX_FMT_RGB24;
    av_frame_get_buffer(frameRgb,0);
    img_convert_ctx = sws_getContext(frameSrc->width,frameSrc->height,(AVPixelFormat)frameSrc->format,
                                    frameRgb->width,frameRgb->height,(AVPixelFormat)frameRgb->format,
                                    SWS_BILINEAR, nullptr, nullptr, nullptr);
}
bool VplView::stop(){
    if(!vpl::RqsiPopFrameHandler::stop()){
        return false;
    }

    if(frameRgb){
        av_frame_free(&frameRgb);
        frameRgb = nullptr;
    }
    if(img_convert_ctx){
        sws_freeContext(img_convert_ctx);
        img_convert_ctx = nullptr;
    }
}
void VplView::onHandle(AVFrame *frameSrc){
    int cvt = sws_scale(img_convert_ctx,frameSrc->data,frameSrc->linesize,0,frameSrc->height,frameRgb->data,frameRgb->linesize);
    emit imgUpdated(frameRgb);
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
}

void VplView::paintEvent(QPaintEvent *){
    if(frameRgb && VPL_STATUS_RUNNING == status){
        QPainter painter(this);

        int width = frameRgb->linesize[0] / 3;
        //1 init img
        QImage img(frameRgb->data[0],width,frameRgb->height,QImage::Format_RGB888);
        QSize rawSize = img.size();
        // QSize targetSize = this->rect().size();

        //2 scale
        // rawSize.scale(targetSize,Qt::KeepAspectRatio);

        //3 for safe
        if(rawSize.width() < 5 || rawSize.height() < 5)
            return;

//        //calculate pos
        // int x = this->rect().width()/2 - rawSize.width()/2;
        // int y = this->rect().height()/2 - rawSize.height()/2;

//        //4 draw
//        painter.drawImage(QRect(x,y,rawSize.width(),rawSize.height()),img);
        // printf("w:%d,h:%d\n",this->rect().width(),this->rect().height());
        painter.drawImage(this->rect(),img);

    }
}
void VplView::onImgUpdated(AVFrame *frame){

    this->update();
}
// void VplView::onRenderThread(){
//     AVFrame *frameRgb = av_frame_alloc();
//     SwsContext *img_convert_ctx = nullptr;
//     int destW = this->width();
//     int destH = this->height();
//     int64_t timeNow = av_gettime();

//     bool isFirst = true;
//     while(isRunningFlag){
//         AVFrame *frameSrc;
//         if(RINQUEUE_RET_OK == rq->pop(&frameSrc)){
//             // VPL_LOG(VPL_LOG_INFO,"pop %p\n",frameSrc)
//             if(isFirst){
//                 //read 1st frame for init
//                 frameRgb->width = destW;
//                 frameRgb->height = destH;
//                 frameRgb->format = AV_PIX_FMT_RGB24;
//                 av_frame_get_buffer(frameRgb,0);
//                 img_convert_ctx = sws_getContext(frameSrc->width,frameSrc->height,(AVPixelFormat)frameSrc->format,
//                                                 frameRgb->width,frameRgb->height,(AVPixelFormat)frameRgb->format,
//                                                 SWS_BILINEAR, nullptr, nullptr, nullptr);
//                 isFirst= false;
//             }

//             int cvt = sws_scale(img_convert_ctx,frameSrc->data,frameSrc->linesize,0,frameSrc->height,frameRgb->data,frameRgb->linesize);


//             av_frame_free(&frameSrc);
//             emit imgUpdated(frameRgb);
//             std::this_thread::sleep_for(std::chrono::milliseconds(25));
//         }
//         else{
//             // std::this_thread::yield();
//             std::this_thread::sleep_for(std::chrono::milliseconds(1));
//         }
        
//     }
//     sws_freeContext(img_convert_ctx);
//     av_frame_free(&frameRgb);
// }
