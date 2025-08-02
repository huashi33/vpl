#ifndef PUSHER_H
#define PUSHER_H
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
}

namespace vpl{
class pusher{
private:
AVFormatContext *outFmtCtx;
AVCodecContext *outCodecCtx;
AVStream *outStream;
// AVPacket pkt;
AVFrame *outFrame;

private:
    static AVFrame* allocPic(AVPixelFormat pix_fmt, int width, int height);



public:
    pusher(/* args */);
    ~pusher();


    int init(const char *url,AVCodecContext *codecCtx);
    
    int write(AVFrame *f);

};
    

    
}




#endif


