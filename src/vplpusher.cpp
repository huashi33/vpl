#include "vplpusher.h"
#include "vpldefine.h"
namespace vpl{
AVFrame *pusher::allocPic(AVPixelFormat pix_fmt, int width, int height){
    AVFrame *picture;
    int ret;

    picture = av_frame_alloc();
    if (!picture)
        return nullptr;

    picture->format = pix_fmt;
    picture->width  = width;
    picture->height = height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, 32);
    if(ret < 0){
        VPL_AVERROR(ret);
        return nullptr;
    }
    

    return picture;
}

vpl::pusher::pusher(){
    outFmtCtx = nullptr;
    outCodecCtx = nullptr;
    outStream = nullptr;
    outFrame = nullptr;
}

vpl::pusher::~pusher(){
    av_write_trailer(outFmtCtx);
    if(nullptr != outCodecCtx){
        avcodec_free_context(&outCodecCtx);
        outCodecCtx=nullptr;
    }



    if(nullptr != outFmtCtx){
        avio_close(outFmtCtx->pb);
        avformat_free_context(outFmtCtx);
        outFmtCtx=nullptr;
    }

    
    
}

int vpl::pusher::init(const char *url,AVCodecContext *codecCtx){
    int r = 1;
    // r = avformat_network_init();
    // if(0 > r){
    //     VPL_LOG(VPL_LOG_ERR0,"can not open: %s",url);
    //     return VPL_RET_PARAM0;
    // }
    r = avformat_alloc_output_context2(&outFmtCtx, NULL, "flv", url);
    VPL_CHECKRET(r,VPL_RET_PARAM0)
    
    const AVCodec *outCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    outStream = avformat_new_stream(outFmtCtx, outCodec);
    VPL_CHECKRET(r,VPL_RET_PARAM0 + 1)
    outStream->time_base = AVRational{1,25};

    
    outCodecCtx = avcodec_alloc_context3(outCodec);
    VPL_CHECKRET(r,VPL_RET_PARAM0 + 2)

    outCodecCtx->codec_id = outCodec->id;
    //码率：影响体积，与体积成正比：码率越大，体积越大；码率越小，体积越小。
    outCodecCtx->bit_rate = 400000; //设置码率 400kps
    /*分辨率必须是2的倍数。 */
    outCodecCtx->width    = 544;
    outCodecCtx->height   = 960;
    

    
    outCodecCtx->time_base     = outStream->time_base;
    outCodecCtx->framerate = AVRational{1,25};
    outCodecCtx->gop_size      = 10; /* 最多每十二帧发射一帧内帧 */
    outCodecCtx->pix_fmt       = AV_PIX_FMT_YUV420P;

    // if (outCodec->id == AV_CODEC_ID_H264)
    //     av_opt_set(c->priv_data, "preset", "slow", 0);


    /* 某些格式希望流头分开。 */
    if (outFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
        outCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;




    AVDictionary *opt = nullptr;
    r = avcodec_open2(outCodecCtx, outCodec, &opt);
    VPL_CHECKRET(r,VPL_RET_PARAM0 + 3)



    // outFrame = allocPic(outCodecCtx->pix_fmt, outCodecCtx->width, outCodecCtx->height);
    // if(!outFrame){
    //     return VPL_RET_MEMERY;
    // }

    r = avcodec_parameters_from_context(outStream->codecpar, outCodecCtx);
    VPL_CHECKRET(r,VPL_RET_PARAM0 + 4)

    //print info
    av_dump_format(outFmtCtx, 0, url, 1);


    //
    r = avio_open(&outFmtCtx->pb, url, AVIO_FLAG_WRITE);
    VPL_CHECKRET(r,VPL_RET_PARAM0 + 5)


    

    
    //写入头部信息
    r = avformat_write_header(outFmtCtx, 0);
    VPL_CHECKRET(r,VPL_RET_PARAM0 + 6)
 

    return r;
}

int vpl::pusher::write(AVFrame *f){
    int r = 1;
    // r = avcodec_send_frame(out_codecCtx, f);
    // if(0 != r){
    //     VPL_AVERROR(r)
    //     return r;
    // }
    // while (0 == avcodec_receive_packet(out_codecCtx, &pkt)) {
    //     av_interleaved_write_frame(outFmtCtx, &pkt);
    //     // av_packet_unref(&pkt);
    // }

    int got_packet = 0;
    AVPacket pkt;
    av_init_packet(&pkt);
    r = avcodec_send_frame(outCodecCtx, f);
    VPL_CHECKRET(r,r)
    static int64_t index = 0;
    while (0 == avcodec_receive_packet(outCodecCtx, &pkt)) {
        // av_packet_rescale_ts(&pkt, outCodecCtx->time_base, outStream->time_base);
        pkt.stream_index = 0;
        av_interleaved_write_frame(outFmtCtx, &pkt);
        av_packet_unref(&pkt);
    }

    return r;
}

}