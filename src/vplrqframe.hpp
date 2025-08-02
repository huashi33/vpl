#ifndef VPL_RQFRAME_HPP
#define VPL_RQFRAME_HPP
#include <stdint.h>
#include <stdlib.h>
#include <atomic>
#include <thread>
#include <functional>
#include "vpldefine.h"
extern "C" {
    #include <libavutil/frame.h>
}



#define RINQUEUE_RET_OK             0 
#define RINQUEUE_RET_ERROR_FULL     1
#define RINQUEUE_RET_ERROR_EMPTY    2


namespace vpl{

class RingQueueAvFrame{
public:
RingQueueAvFrame(){
    data = nullptr;
}
RingQueueAvFrame(const vpl::RingQueueAvFrame &src){
    itemNum = src.itemNum;
    head.store(src.head.load());
    tail.store(src.tail.load());
    data = src.data;
}

~RingQueueAvFrame(){
    if(data){
        free(data);
        data = nullptr;
    }
}

int init(int itemNum){
    this->itemNum = itemNum;
    head.store(0);
    tail.store(0);
    data = (AVFrame **)calloc(itemNum,sizeof(AVFrame *));
    return RINQUEUE_RET_OK;
}
int push(AVFrame *d){
    int h = head.load();
    int t = tail.load();

    int nextT = (t+1) & (itemNum -1);
    if(nextT == h){
        return RINQUEUE_RET_ERROR_FULL;
    }

    data[t] = d;
    tail.store(nextT);
    return RINQUEUE_RET_OK;
}
int pop(AVFrame **d){
    int h = head.load();
    int t = tail.load();
    if(t == h){
        return RINQUEUE_RET_ERROR_EMPTY;
    }
    *d = data[h];

    int nextH = (h+1) & (itemNum -1);
    head.store(nextH);

    return RINQUEUE_RET_OK;
}


protected:
    int itemNum;
    std::atomic<int> head;
    std::atomic<int> tail;
    AVFrame **data;
};


class RingQueueAvFrameHandler
{
protected:
vpl::RingQueueAvFrame *rq;
std::thread *threadHandle;
VplStatus_t status;
bool isFirst;

public:
RingQueueAvFrameHandler(){
    rq = nullptr;
    status = VPL_STATUS_NONE;
    isFirst = true;
    threadHandle = nullptr;
}
~RingQueueAvFrameHandler(){
    status = VPL_STATUS_NONE;
}
void setRq(RingQueueAvFrame *rq){
    this->rq = rq;
    status = VPL_STATUS_INITED;
}
virtual void onHandleStart(AVFrame *frameSrc) = 0;
virtual void onHandleStop() = 0;
virtual void onHandle(AVFrame *frameSrc) = 0;
void onHandleBase(){
    status = VPL_STATUS_RUNNING;
    AVFrame *frameSrc;
    while(VPL_STATUS_RUNNING == status){
        
        if(RINQUEUE_RET_OK == rq->pop(&frameSrc)){
            if(!isFirst){
                onHandle(frameSrc);
            }
            else{
                onHandleStart(frameSrc);
                onHandle(frameSrc);
                isFirst = false;
            }
            av_frame_free(&frameSrc);
        }
        else{
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    while (RINQUEUE_RET_OK == rq->pop(&frameSrc)){
        av_frame_free(&frameSrc);
    }
    

    onHandleStop();
    status = VPL_STATUS_NONE;
    isFirst = true;
}
void start(){
    if(VPL_STATUS_INITED == status){
        threadHandle = new std::thread(std::bind(&RingQueueAvFrameHandler::onHandleBase,this));
        
    }
}
void stop(){
    if(VPL_STATUS_RUNNING == status){
        status = VPL_STATUS_STOP;
        threadHandle->join();
        delete threadHandle;
        threadHandle = nullptr;
    }
}
VplStatus_t getStatus(){
    return status;
}
};



}




#endif