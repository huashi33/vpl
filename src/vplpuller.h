#ifndef PULLER_H
#define PULLER_H
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavcodec/codec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/time.h>
    #include <libavutil/imgutils.h>
}
#include "vpldefine.h"
#include "vplringqueue.hpp"
#include <vector>
#include <thread>



namespace vpl{


class puller{
private:


double avgFrameRate; //帧率
double allTime; //总时间
VplStatus_t status;
VplFrameParam_t fp;

AVFormatContext *format_ctx;
AVStream *stream;
AVCodecContext *codeCtx;

std::thread *pullThread;
std::vector<RingQueueSpscItem *> rqs;
private:

    void onPullThread();
public:
    puller();
    ~puller();

    int start();
    void stop();

    int open(const char *url);
    int close();
    
    //for rq
    int addRq(RingQueueSpscItem *rq);
    int removeAllRq();
    int getRqCount();

    //
    double getPts(AVFrame *f);
    VplStatus_t getStatus();
    VplFrameParam_t getFrameParam();
protected:
    int read(AVFrame *f,AVPacket *pkt);
    const AVStream *getStream();
    AVCodecContext *getCodeCtx();



};



}

#endif