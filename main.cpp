#include "mainwindow.h"
#include <QApplication>
#include "libavutil/version.h"
#include "opencv2/core.hpp"
#include <openvino/openvino.hpp>
#include "src/vplpuller.h"
#include "src/vplpusher.h"
#include "src/vpldefine.h"
#include "src/vpltool.hpp"
#include "src/vplringqueue.hpp"
extern "C" {
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
    #include <libavutil/time.h>
    #include <libavutil/opt.h>
}


static void printLibVersion(){
    printf("ffmpeg-avutil version:%s\n",AV_STRINGIFY(LIBAVUTIL_VERSION));
    printf("opencv version:%s\n",CV_VERSION);
    printf("openvino version:%s\n",ov::get_openvino_version().description);

}


static int testPull1(int argc, char *argv[]){
    int r = 1;
    const char *url = argv[1];
    vpl::puller p;
    r = p.open(url);
    if(0 != r){
        return r;
    }

    // vpl::StopWatch sw;
    AVFrame *f = av_frame_alloc();
    while (0 == r){
        // sw.start();
        // r = p.read(f);
        // sw.stop("read a frame need");
        double pts = p.getPts(f);
        // VPL_LOG(VPL_LOG_INFO,"[%lld]pts:%.3f",f->pts,pts);
    }
    
    av_frame_free(&f);
    return r;
}

/*
static int testPush1(int argc, char *argv[]){
    int r = 1;
    const char *url = argv[1];
    vpl::puller myPuller;
    r = myPuller.open(url);
    if(0 != r){
        return r;
    }


    const char *outUrl = argv[2];
    vpl::pusher myPusher;
    // const AVStream *inStream = myPuller.getStream();
    r = myPusher.init(outUrl,myPuller.getCodeCtx());
    if(0 != r){
        return r;
    }

    // vpl::StopWatch sw;
    AVFrame *f = av_frame_alloc();
    while (0 == r){
        // sw.start();
        // r = myPuller.read(f);
        // sw.stop("read a frame need");
        double pts = myPuller.getPts(f);
        VPL_LOG(VPL_LOG_INFO,"[%lld]pts:%.3f",f->pts,pts);
        printf("\r");
        myPusher.write(f);
        // av_usleep(25*1000);
        
    }
    
    av_frame_free(&f);
    return r;
}

static int testrq(int argc, char *argv[]){
    int prodNum = atoi(argv[1]);
    int cusNum = atoi(argv[2]);


    bool isFinish = false;

    vpl::RingQueue rq;
    rq.init(sizeof(int),128);
    //cunsumer
    auto funcConsumer = [&](){
        int d = 0;
        int index = 0;
        while (!isFinish){
            if(RINQUEUE_RET_OK == rq.pop(&d)){
                ++index;
                printf("pop:[%d][%d]:%d\n",std::this_thread::get_id(),index,d);
            }
            else{
                std::this_thread::yield();
            }
        }
    };
    std::vector<std::thread> threads;
    for(int i = 0;i < cusNum;++i){
        threads.emplace_back(funcConsumer);
    }

    //producter
    auto funcProduct = [&](){
        for(int i = 0;i < 50;++i){
            while (RINQUEUE_RET_OK != rq.push(&i)){
                std::this_thread::yield();
            }
            printf("push:%d:%d\n",std::this_thread::get_id(),i);
        }
    };
    for(int i = 0;i < prodNum;++i){
        threads.emplace_back(funcProduct);
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    isFinish = true;
    for(int i = 0;i <prodNum+ cusNum;++i){
        threads[i].join();
    }
    return 0;
}

static int testrqs(int argc, char *argv[]){
    int prodNum = atoi(argv[1]);
    int cusNum = atoi(argv[2]);


    bool isFinish = false;

    vpl::RingQueueSimple rqs(128);
    //cunsumer
    auto funcConsumer = [&](){
        int d = 0;
        int index = 0;
        while (!isFinish){
            if(rqs.pop(&d,sizeof(d))){
                ++index;
                printf("pop:[%d][%d]:%d\n",std::this_thread::get_id(),index,d);
            }
            else{
                std::this_thread::yield();
            }
        }
    };
    std::vector<std::thread> threads;
    for(int i = 0;i < cusNum;++i){
        threads.emplace_back(funcConsumer);
    }

    //producter
    auto funcProduct = [&](){
        for(int i = 0;i < 50;++i){
            while (!rqs.push(&i,sizeof(i))){
                std::this_thread::yield();
            }
            printf("push:%d:%d\n",std::this_thread::get_id(),i);
        }
    };
    for(int i = 0;i < prodNum;++i){
        threads.emplace_back(funcProduct);
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    isFinish = true;
    for(int i = 0;i <prodNum+ cusNum;++i){
        threads[i].join();
    }
    return 0;
}
*/

class FrameWriter{
private:
SwsContext *swsCtx;
AVFrame *frameJpg;
AVCodecContext *enCtx;
public:
FrameWriter(int w,int h,int inFormat){
    swsCtx = nullptr;
    frameJpg = nullptr;
    enCtx = nullptr;

    //frame
    frameJpg = av_frame_alloc();
    frameJpg->format = AV_PIX_FMT_YUVJ420P;
    frameJpg->width = w ;
    frameJpg->height = h;
    av_frame_get_buffer(frameJpg,32);
    //sws
    swsCtx= sws_getContext(frameJpg->width,frameJpg->height,(AVPixelFormat)inFormat,
                            frameJpg->width,frameJpg->height,(AVPixelFormat)frameJpg->format,
                            SWS_BILINEAR, nullptr, nullptr, nullptr);
    //codectx
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);  
    enCtx = avcodec_alloc_context3(codec);
    enCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
    enCtx->time_base =  {1, 25}; 
    enCtx->width  = frameJpg->width;             
    enCtx->height = frameJpg->height;
    avcodec_open2(enCtx, codec, NULL);   
}
~FrameWriter(){
    if(swsCtx){
        sws_freeContext(swsCtx);
        swsCtx = nullptr;
    }
    if(frameJpg){
        av_frame_free(&frameJpg);
        frameJpg = nullptr;
    }
    if(enCtx){
        avcodec_send_frame(enCtx,nullptr);
        avcodec_close(enCtx);
        avcodec_free_context(&enCtx);
    }
}

void write(AVFrame *f,const char *fileName){
    int cvt = sws_scale(swsCtx,f->data,f->linesize,0,f->height,frameJpg->data,frameJpg->linesize);
    if(0<= avcodec_send_frame(enCtx, frameJpg)){
        AVPacket *pkt = av_packet_alloc();
        FILE *f = fopen(fileName, "wb");
        while (0 <= avcodec_receive_packet(enCtx, pkt)){
            fwrite(pkt->data, 1, pkt->size, f);      
            av_packet_unref(pkt);
        }
        fclose(f);
        av_packet_free(&pkt);
        
    }
} 

};
class VideoWriter{
private:
SwsContext *swsCtx;
AVFrame *frameDest;
AVCodecContext *enCtx;
FILE *outFile;
int pts; 
public:
VideoWriter(int w,int h,int inFormat,const char *fileName){
    swsCtx = nullptr;
    frameDest = nullptr;
    enCtx = nullptr;
    outFile = nullptr;
    pts = 0;

    //frame
    frameDest = av_frame_alloc();
    frameDest->format = AV_PIX_FMT_YUV420P;
    frameDest->width = w ;
    frameDest->height = h;
    av_frame_get_buffer(frameDest,0);
    //sws
    swsCtx= sws_getContext(w,h,(AVPixelFormat)inFormat,w,h,(AVPixelFormat)frameDest->format,
                           SWS_BILINEAR, nullptr, nullptr, nullptr);
    //codectx
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_MPEG1VIDEO);  
    enCtx = avcodec_alloc_context3(codec);
    enCtx->width  = w;             
    enCtx->height = h;
    enCtx->pix_fmt = (AVPixelFormat)frameDest->format;
    enCtx->time_base =  {1, 30}; 
    enCtx->framerate = {25, 1};
    enCtx->bit_rate = 400000;

    avcodec_open2(enCtx, codec, NULL); 

    outFile = fopen(fileName, "wb");
}
~VideoWriter(){
    if(swsCtx){
        sws_freeContext(swsCtx);
        swsCtx = nullptr;
    }
    if(frameDest){
        av_frame_free(&frameDest);
        frameDest = nullptr;
    }
    if(enCtx){
        avcodec_send_frame(enCtx,nullptr);
        avcodec_close(enCtx);
        avcodec_free_context(&enCtx);
    }

    if(outFile){
        uint8_t endcode[] = { 0, 0, 1, 0xb7 };
        fwrite(endcode, 1, sizeof(endcode), outFile);
        fclose(outFile);
        outFile = nullptr;
    }
}
void write(AVFrame *f){
    int cvt = sws_scale(swsCtx,f->data,f->linesize,0,f->height,frameDest->data,frameDest->linesize);
    frameDest->pts = pts++;
    if(0<= avcodec_send_frame(enCtx, frameDest)){
        AVPacket *pkt = av_packet_alloc();
        while (0 <= avcodec_receive_packet(enCtx, pkt)){
            fwrite(pkt->data, 1, pkt->size, outFile);      
            av_packet_unref(pkt);
        }
        av_packet_free(&pkt);
        
    }
} 
};
class StreamWriter{
private:
SwsContext *swsCtx;
AVFrame *frameDest;
AVCodecContext *enCtx;
AVFormatContext *outFormatCtx;
AVStream *outStream;
int pts; 
public:
void initRtmp(int w,int h,int inFormat,const char *url){
    int r = 0;
    r = avformat_network_init(); 
    r = avformat_alloc_output_context2(&outFormatCtx, nullptr, "flv", url); 
    // r = avformat_alloc_output_context2(&outFormatCtx, nullptr, "rtsp", url); 
    //codec
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);  

    //codectx
    enCtx = avcodec_alloc_context3(codec);
    enCtx->width  = w;             
    enCtx->height = h;
    enCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    enCtx->time_base =  {1, 30}; 
    enCtx->framerate = {30, 1};
    enCtx->gop_size = 12;
    enCtx->bit_rate = 400000;

    if (outFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
        enCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;


    r = avcodec_open2(enCtx, codec, NULL); 



    //stream
    outStream =avformat_new_stream(outFormatCtx,codec);
    outStream->time_base = enCtx->time_base;
    avcodec_parameters_from_context(outStream->codecpar,enCtx);
    // r = av_opt_set(outFormatCtx->priv_data,"rtsp_transport","tcp",0);
    // r = av_opt_set(outFormatCtx->priv_data,"max_delay","500000",0);


    if (!(outFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        r = avio_open(&outFormatCtx->pb, url, AVIO_FLAG_WRITE); 
    }

    r = avformat_write_header(outFormatCtx, nullptr);  
}
void initRtsp(int w,int h,int inFormat,const char *url){
    int r = 0;
    r = avformat_network_init(); 
    r = avformat_alloc_output_context2(&outFormatCtx, nullptr, "rtsp", url); 
    //codec
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);  

    //codectx
    enCtx = avcodec_alloc_context3(codec);
    enCtx->width  = w;             
    enCtx->height = h;
    enCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    enCtx->time_base =  {1, 30}; 
    enCtx->framerate = {30, 1};
    enCtx->gop_size = 12;
    enCtx->bit_rate = 400000;

    if (outFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
        enCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;


    r = avcodec_open2(enCtx, codec, NULL); 

    //stream
    outStream =avformat_new_stream(outFormatCtx,codec);
    outStream->time_base = enCtx->time_base;
    avcodec_parameters_from_context(outStream->codecpar,enCtx);
    // r = av_opt_set(outFormatCtx->priv_data,"rtsp_transport","tcp",0);
    // r = av_opt_set(outFormatCtx->priv_data,"max_delay","500000",0);


    if (!(outFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        r = avio_open(&outFormatCtx->pb, url, AVIO_FLAG_WRITE); 
    }

    r = avformat_write_header(outFormatCtx, nullptr);  

}
StreamWriter(int w,int h,int inFormat,const char *url){
    swsCtx = nullptr;
    frameDest = nullptr;
    enCtx = nullptr;
    outFormatCtx = nullptr;
    outStream = nullptr;
    pts = 0;
    // const char *rtmp_url = "rtmp://localhost/live/stream";
    // const char *rtsp_url = "rtsp://localhost/live/stream";
    if(strstr(url,"rtmp")){
        initRtmp(w,h,inFormat,url);
    }
    else if(strstr(url,"rtsp")){
        initRtsp(w,h,inFormat,url);
    }

    
    //frame to codec
    frameDest = av_frame_alloc();
    frameDest->format = enCtx->pix_fmt;
    frameDest->width = w ;
    frameDest->height = h;
    av_frame_get_buffer(frameDest,0);
    //sws
    swsCtx= sws_getContext(w,h,(AVPixelFormat)inFormat,w,h,(AVPixelFormat)frameDest->format,
                            SWS_BILINEAR, nullptr, nullptr, nullptr);

    
    

}
~StreamWriter(){
    if(swsCtx){
        sws_freeContext(swsCtx);
        swsCtx = nullptr;
    }
    if(frameDest){
        av_frame_free(&frameDest);
        frameDest = nullptr;
    }
    if(enCtx){
        avcodec_send_frame(enCtx,nullptr);
        avcodec_close(enCtx);
        avcodec_free_context(&enCtx);
        enCtx = nullptr;
    }

    if(outFormatCtx){
        av_write_trailer(outFormatCtx);
        if (!(outFormatCtx->oformat->flags & AVFMT_NOFILE)) {
            avio_close(outFormatCtx->pb);
        }
        avformat_free_context(outFormatCtx);
        outFormatCtx = nullptr;
    }
}

void write(AVFrame *f){
    int cvt = sws_scale(swsCtx,f->data,f->linesize,0,f->height,frameDest->data,frameDest->linesize);
    frameDest->pts = av_rescale_q(pts++,enCtx->time_base,outStream->time_base); 
    if(0<= avcodec_send_frame(enCtx, frameDest)){
        AVPacket *pkt = av_packet_alloc();
        while (0 <= avcodec_receive_packet(enCtx, pkt)){
            
            pkt->stream_index = outStream->index;
            // av_packet_rescale_ts(pkt, enCtx->time_base, outStream->time_base);
            av_interleaved_write_frame(outFormatCtx, pkt); 
            av_packet_unref(pkt);
        }
        av_packet_free(&pkt);
        
    }
} 

};



static void updateRgbFrame(int count,AVFrame *f){
    av_frame_make_writable(f);
    const int cubeLen = 100;
    int colMax = f->width / cubeLen;
    int rowMax = f->height / cubeLen;
    int row = count / colMax ;
    row = row % rowMax;
    int col = count % colMax;

    uint8_t *head = f->data[0] + cubeLen*row*f->linesize[0] + 3*cubeLen*col;
    for(int i = 0;i < cubeLen;++i){
        for(int j = 0;j < cubeLen;++j){
            uint8_t *dest = head + i*f->linesize[0] + 3*j;
            dest[0] = UINT8_MAX;
            // dest[1] = UINT8_MAX;
            // dest[2] = UINT8_MAX;
        }
    }
}
static int test1FrameWrite(int argc, char *argv[]){
    int num = atoi(argv[1]);

    const int w = 800;
    const int h = 600;
    const int format = AV_PIX_FMT_RGB24;
    FrameWriter *wr = new FrameWriter(w,h,format);
    static int count = 0;
    for (size_t i = 0; i < num; i++){
        AVFrame *frameSrc = av_frame_alloc();
        frameSrc->format = format;
        frameSrc->width = w;
        frameSrc->height = h;
        av_frame_get_buffer(frameSrc,0);
        updateRgbFrame(i,frameSrc);
        
        char filename[32] = {0};
        snprintf(filename,sizeof(filename) -1,"%d.jpg",count++);
        wr->write(frameSrc,filename);
        av_frame_free(&frameSrc);
    }
    return 1;
}
static int testNFrameWrite(int argc, char *argv[]){
    int num = atoi(argv[1]);
    const char *filename = argv[2];
    const int w = 800;
    const int h = 600;
    const int format = AV_PIX_FMT_RGB24;
    VideoWriter *wr = new VideoWriter(w,h,format,filename);
    static int count = 0;
    for (size_t i = 0; i < num; i++){
        AVFrame *frameSrc = av_frame_alloc();
        frameSrc->format = format;
        frameSrc->width = w;
        frameSrc->height = h;
        av_frame_get_buffer(frameSrc,0);
        updateRgbFrame(i,frameSrc);
        
        wr->write(frameSrc);
        av_frame_free(&frameSrc);
    }
    delete wr;


  
    return 1;
}
static int testSreamWrite(int argc, char *argv[]){
    int num = atoi(argv[1]);
    const char *filename = argv[2];
    const int w = 800;
    const int h = 600;
    const int format = AV_PIX_FMT_RGB24;
    StreamWriter *wr = new StreamWriter(w,h,format,filename);
    static int count = 0;
    for (size_t i = 0; i < num; i++){
        AVFrame *frameSrc = av_frame_alloc();
        frameSrc->format = format;
        frameSrc->width = w;
        frameSrc->height = h;
        av_frame_get_buffer(frameSrc,0);
        updateRgbFrame(i,frameSrc);
        
        wr->write(frameSrc);
        av_frame_free(&frameSrc);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    delete wr;


  
    return 1;
}



static int test1FrameWriteWithoutAllocFrame(int argc, char *argv[]){
    int num = atoi(argv[1]);

    const int w = 800;
    const int h = 600;
    const int format = AV_PIX_FMT_RGB24;
    FrameWriter *wr = new FrameWriter(w,h,format);
    static int count = 0;

    int allocSize = av_image_get_buffer_size((AVPixelFormat)format,w,h,32);
    uint8_t *allocData = (uint8_t *)av_malloc(allocSize);
    AVFrame *frameSrc = av_frame_alloc();
    frameSrc->format = format;
    frameSrc->width = w;
    frameSrc->height = h;
    av_image_fill_arrays(frameSrc->data,frameSrc->linesize,allocData,(AVPixelFormat)format,w,h,32);
    for (size_t i = 0; i < num; i++){
        // AVFrame *frameSrc = av_frame_alloc();
        // frameSrc->format = format;
        // frameSrc->width = w;
        // frameSrc->height = h;
        // av_frame_get_buffer(frameSrc,0);
        updateRgbFrame(i,frameSrc);
        
        char filename[32] = {0};
        snprintf(filename,sizeof(filename) -1,"%d.jpg",count++);
        wr->write(frameSrc,filename);
        // av_frame_free(&frameSrc);
    }
    return 1;
}





int main(int argc, char *argv[]){
    int r = 1;
    const char *url = argv[1];
    printLibVersion();

    // r = testSreamWrite(argc,argv);
    // test1FrameWriteWithoutAllocFrame(argc,argv);





    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    r = a.exec();
    return r;
}
