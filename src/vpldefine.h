#ifndef VPL_DEFINE_H
#define VPL_DEFINE_H
#include <stdio.h>



//ret code
#define VPL_RET_OK          0x00000000
#define VPL_RET_MEMERY      0x00000001
#define VPL_RET_NOTINITED   0x00000002
#define VPL_RET_HASEXIST    0x00000003
#define VPL_RET_PARAM0      0x00000100


typedef enum VplStatus_{
    VPL_STATUS_NONE,
    VPL_STATUS_INITED,
    VPL_STATUS_RUNNING,
    VPL_STATUS_STOP
}VplStatus_t;
typedef struct AVFrameParam_{
    int width;
    int height;
    int format;
    int algn;
}VplFrameParam_t;

#define VPL_LOG_INFO 3
#define VPL_LOG_WARN 2
#define VPL_LOG_ERR0 1
#define VPL_LOG(LEVEL,FMT,...) printf("[%s:%d]:" FMT " ",__FUNCTION__,__LINE__,##__VA_ARGS__);

#define VPL_AVERROR(ERRORNUM) {char buf[1024]={0};\
                              av_strerror(ERRORNUM, buf, sizeof(buf));\
                              VPL_LOG(VPL_LOG_ERR0,"%s",buf)}\


#define VPL_CHECKRET(ERRNO,RET) if(0 > ERRNO){VPL_AVERROR(ERRNO)\
                                        return RET;}\



// int avError(int errNum) {
//     char buf[1024];
//     //获取错误信息
//     av_strerror(errNum, buf, sizeof(buf));
    
//     return -1;
// }

#endif