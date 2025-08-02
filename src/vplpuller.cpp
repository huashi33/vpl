#include "vplpuller.h"
#include "vpldefine.h"
#include "vplringqueue.hpp"
#include <functional>
#include <thread>
#include "vpltool.hpp"
extern "C" {
    #include <libavutil/imgutils.h>
}
namespace vpl{



puller::puller(){
    status = VPL_STATUS_NONE;
    format_ctx = nullptr;
    stream = nullptr;
    codeCtx = nullptr;
    pullThread = nullptr;
}


puller::~puller(){
}

int puller::start(){
    if(VPL_STATUS_INITED != status){
        return VPL_RET_NOTINITED;
    }
    pullThread = new std::thread(std::bind(&puller::onPullThread,this));
    return VPL_RET_OK;
}

void puller::stop(){
    if(VPL_STATUS_RUNNING == status){
        status = VPL_STATUS_STOP;
        pullThread->join();
        delete pullThread;
    }
}
VplStatus_t puller::getStatus(){
    return status;
}

void puller::onPullThread(){

    status = VPL_STATUS_RUNNING;
    int hasPushed[16] = {0};
    AVFrame *srcFrame=av_frame_alloc();
    AVPacket *pkt = av_packet_alloc();
    AVFrame *picTmp = av_frame_alloc();
    picTmp->width = fp.width;
    picTmp->height = fp.height;
    picTmp->format = fp.format;

    // av_frame_get_buffer(tmp,0);

    vpl::StopWatch sw;
    sw.start();
    int c = 0;
    while (VPL_STATUS_RUNNING == status && 0 <= av_read_frame(format_ctx, pkt)){
        if(VPL_RET_OK == read(srcFrame,pkt)){
            ++c;
            bool hasPushAll = false;
            memset(hasPushed,0,sizeof(int)*rqs.size());
            while (VPL_STATUS_RUNNING == status && !hasPushAll){
                for(int i = 0;i < rqs.size();++i){
                    uint8_t *picDataTmp = (uint8_t *)rqs[i]->beginePush();
                    if(!hasPushed[i] && picDataTmp){
                        av_image_fill_arrays(picTmp->data,picTmp->linesize,picDataTmp,
                            (AVPixelFormat)fp.format,fp.width,fp.height,fp.algn);
                        av_frame_copy(picTmp,srcFrame);
                        hasPushed[i] = 1;
                        rqs[i]->endPush();
                    }
                }

                hasPushAll = true;
                for(int i = 0;i < rqs.size();++i){
                    if(!hasPushed[i]){
                        hasPushAll = false;
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                        break;
                    }
                }
            }
            av_frame_unref(srcFrame);
           
        }
        av_packet_unref(pkt);
    }
    sw.stop("span");
    printf("c:%d\n",c);


    
    while (0 <= avcodec_send_packet(codeCtx,nullptr)){
        while(0 <= avcodec_receive_frame(codeCtx, srcFrame))
            av_frame_unref(srcFrame);
    }
    


    // free(hasPushed);
    av_frame_unref(srcFrame);
    av_frame_free(&srcFrame);
    av_packet_unref(pkt);
    av_packet_free(&pkt);
    status = VPL_STATUS_NONE;
}
int puller::open(const char *url){
    int ret = VPL_RET_PARAM0;

    format_ctx = avformat_alloc_context();
    if(avformat_open_input(&format_ctx, url, nullptr, nullptr) != 0){
         VPL_LOG(VPL_LOG_ERR0,"can not open: %s",url);
         return VPL_RET_PARAM0;
    }
    if(avformat_find_stream_info(format_ctx, nullptr) != 0){
        VPL_LOG(VPL_LOG_ERR0,"%s","can not get stream info")
        return VPL_RET_PARAM0 + 1;
    }

    for(unsigned int i = 0; i < format_ctx->nb_streams; ++i){
        AVStream *stream = format_ctx->streams[i];
        if(stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){

            
            auto codeid = avcodec_find_decoder(stream->codecpar->codec_id);
            codeCtx = avcodec_alloc_context3(codeid);
            if(avcodec_parameters_to_context(codeCtx,stream->codecpar)<0){
                VPL_LOG(VPL_LOG_ERR0,"cannot alloc codec context")
                return VPL_RET_PARAM0 + 2;
            }
            
            
            codeCtx->pkt_timebase = stream->time_base;
            if(avcodec_open2(codeCtx,codeid,nullptr)!=0){
                VPL_LOG(VPL_LOG_ERR0,"video decoder open fail\n")
                return VPL_RET_PARAM0 + 3;
            }
            this->stream = stream;
        

            allTime = av_q2d(stream->time_base)*stream->duration;
            avgFrameRate = av_q2d(stream->avg_frame_rate);

            //
            fp.width = stream->codecpar->width;
            fp.height = stream->codecpar->height;
            fp.format = stream->codecpar->format;
            fp.algn = 1;

            VPL_LOG(VPL_LOG_INFO,
                   "inited,url:%s,w %d,h %d,avgFrameRate %.3f,allTime %.3f\n",
                   url, fp.width, fp.height, avgFrameRate,allTime)
           ret = VPL_RET_OK;
           
        }
    }
    
    if(VPL_RET_OK == ret){
        av_dump_format(format_ctx, 0, url, 0);
        // strncpy(this->url,url,sizeof(this->url) -1);
        status = VPL_STATUS_INITED ;


    }

    VPL_LOG(VPL_LOG_ERR0,"[%08X]readInfo(%s)\n",ret,url);
    return ret;
}

int puller::close(){
    if(codeCtx){
        avcodec_close(codeCtx);
        avcodec_free_context(&codeCtx);
        codeCtx = nullptr;
    }


    if(format_ctx){
        avformat_close_input(&format_ctx);
        avformat_free_context(format_ctx);
        format_ctx = nullptr;
    }
    return 0;
}

int puller::addRq(RingQueueSpscItem *rq){
    // int allocSize = av_image_get_buffer_size((AVPixelFormat)fp.format,fp.width,fp.height,fp.algn);
    // RingQueueSpscItem *rq = new RingQueueSpscItem(allocSize,4);
    // rq->setUserData(&fp);
    rqs.emplace_back(rq);
    return 1;
}
int puller::removeAllRq(){
    rqs.clear();
    return 1;
}

int puller::getRqCount(){
    return rqs.size();
}


int puller::read(AVFrame *f,AVPacket *pkt){
    int ret = 1;
    

    // while (0 <= av_read_frame(format_ctx, pkt)){
        if(this->stream->index == pkt->stream_index){
            ret = avcodec_send_packet(codeCtx,pkt);
            if(0 > ret){
                return ret;
            }

            ret = avcodec_receive_frame(codeCtx, f);
            return ret;
        }
        // av_packet_unref(pkt);
    // }
    // av_packet_unref(pkt);
    return ret;
}

double puller::getPts(AVFrame *f){
    double pts = (f->pts == AV_NOPTS_VALUE) ? NAN : f->pts * av_q2d(stream->time_base);
    return pts;
}

const AVStream *puller::getStream(){
    return stream;
}
AVCodecContext *puller::getCodeCtx(){
    return codeCtx;
}
// int puller::getWidth(){
//     return width;
// }
// int puller::getHeight(){
//     return height;
// }
VplFrameParam_t puller::getFrameParam(){
    return fp;
}
}