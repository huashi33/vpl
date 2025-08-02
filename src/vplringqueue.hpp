#ifndef RINGQUEUE_H
#define RINGQUEUE_H
#include <stdint.h>
#include <stdlib.h>
#include <atomic>
#include <thread>
#include <vector>
#include <functional>
#include "vpldefine.h"
extern "C" {
    #include <libavutil/frame.h>
    #include <libavutil/imgutils.h>
}


//for ret
#define RINQUEUE_RET_OK             0 
#define RINQUEUE_RET_ERROR_FULL     1
#define RINQUEUE_RET_ERROR_EMPTY    2

#define RINQUEUE_MIN(a,b)  ((a) > (b) ? (b) : (a))
namespace vpl{

class RingQueueSpscByte{
private:
uint32_t head;
uint32_t tail;
uint32_t size;
char    *data;
public:
    //size must be 2^n
    RingQueueSpscByte(uint32_t size){
        uint32_t trueSize = 2;
        while (trueSize < size){
            trueSize = trueSize <<1;
        }
        this->size = trueSize;
        // head = tail = UINT32_MAX - 50;
        data = (char *)malloc(trueSize);
            
        
    }
    ~RingQueueSpscByte(){
        free(data);
    }

    uint32_t push(void *d,uint32_t len){
        uint32_t l = RINQUEUE_MIN(len,size - tail + head);
        uint32_t realTail = tail&(size -1);
        uint32_t l1 = RINQUEUE_MIN(l,size - realTail);
        memcpy(data+realTail,d,l1);//firat
        memcpy(data,((char *)d)+l1,l - l1);//second
        tail+=l;
        return l;
    }
    uint32_t pop(void *d,uint32_t len){
        uint32_t l = RINQUEUE_MIN(len,tail - head);
        uint32_t realHead = head&(size -1);
        uint32_t l1 = RINQUEUE_MIN(l,size - realHead);
        memcpy(d,data+realHead,l1);//firat
        memcpy(((char *)d)+l1,data,l - l1);//second
        head+=l;
        return l;
    }
};
class RingQueueSpscItem{
uint32_t mHead;
uint32_t mTail;
uint32_t mItemSize;
uint32_t mItemCount;
char    *mData;
void    *userData;
public:
RingQueueSpscItem(uint32_t itemSize,uint32_t itemCount){
    //size must be 2^n
    uint32_t trueItemCount = 2;
    while (trueItemCount < itemCount){
        trueItemCount = trueItemCount <<1;
    }
    this->mItemSize = itemSize;
    this->mItemCount = trueItemCount;
    
    // mHead = mTail = UINT32_MAX - 50;//for debug
    mData = (char *)malloc(itemSize * itemCount);
    if(!mData){
        printf("RingQueueItemSpsc malloc error\n");
        exit(0);
    }
}
~RingQueueSpscItem(){
    if(mData){
        free(mData);
    }
}

void *beginePush(){
    char *rData = nullptr;
    if(mItemCount - mTail + mHead){
        uint32_t realTail = mTail&(mItemCount -1);
        rData = mData+realTail*mItemSize;
    }
    return rData;
}
void endPush(){
    ++mTail;
}

void *beginePop(){
    char *rData = nullptr;
    if(mTail - mHead){
        uint32_t realHead = mHead&(mItemCount -1);
        rData = mData+realHead*mItemSize;
    }
    return rData;
}
void endPop(){
    ++mHead;
}

void setUserData(void *d){
    userData = d;
}
void *getUserData(){
    return userData;
}



};


class RqsiProvider{
private:
std::vector<RingQueueSpscItem *> rqs;


public:
RqsiProvider(){

}
~RqsiProvider(){
    
}
RingQueueSpscItem* add(VplFrameParam_t *fp){
    int allocSize = av_image_get_buffer_size((AVPixelFormat)fp->format,fp->width,fp->height,fp->algn);
    RingQueueSpscItem *rq = new RingQueueSpscItem(allocSize,4);
    rq->setUserData(fp);
    rqs.emplace_back(rq);
    return rq;
}

void removeAll(){
    for (size_t i = 0; i < rqs.size(); i++){
        delete rqs[i];
    }
    rqs.clear();
}

int count(){
    return rqs.size();
}
};








class RqsiPopFrameHandler{
protected:
    RingQueueSpscItem *rq;
    VplFrameParam_t vplFrameParam;
    std::thread *threadHandle;
    VplStatus_t status;
    AVFrame *frameSrc;
public:
RqsiPopFrameHandler(){
    rq = nullptr;
    status = VPL_STATUS_NONE;
    threadHandle = nullptr;
    frameSrc = nullptr;
}
~RqsiPopFrameHandler(){
    status = VPL_STATUS_NONE;
}


void setRq(RingQueueSpscItem *rq){
    this->rq = rq;
    memcpy(&vplFrameParam,rq->getUserData(),sizeof(vplFrameParam));
    status = VPL_STATUS_INITED;
}
virtual bool start(){
    if(VPL_STATUS_INITED == status){
        VplFrameParam_t *fp = &vplFrameParam;
        //init frame

        // int allocSize = av_image_get_buffer_size((AVPixelFormat)fp->format,fp->width,fp->height,32);
        // uint8_t *allocData = (uint8_t *)av_malloc(allocSize);
        frameSrc = av_frame_alloc();
        frameSrc->format = (AVPixelFormat)fp->format;
        frameSrc->width = fp->width;
        frameSrc->height = fp->height;

        threadHandle = new std::thread(std::bind(&RqsiPopFrameHandler::onHandleBase,this));
        return true;
    }
    return false;

}
virtual bool stop(){
    if(VPL_STATUS_RUNNING == status){
        status = VPL_STATUS_STOP;
        threadHandle->join();
        delete threadHandle;
        threadHandle = nullptr;
        av_frame_free(&frameSrc);
        frameSrc = nullptr;
        return true;
    }
    return false;
}
VplStatus_t getStatus(){
    return status;
}

void onHandleBase(){
    status = VPL_STATUS_RUNNING;
    VplFrameParam_t *fp = &vplFrameParam;
    while(VPL_STATUS_RUNNING == status){
        uint8_t *picData = (uint8_t *)rq->beginePop();
        if(picData){
            av_image_fill_arrays(frameSrc->data,frameSrc->linesize,picData,
                                (AVPixelFormat)fp->format,fp->width,fp->height,fp->algn);

            onHandle(frameSrc);
            rq->endPop();
        }
        else{
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    status = VPL_STATUS_NONE;
}
virtual void onHandle(AVFrame *frameSrc) = 0;


};



} // namespace vpl

#endif