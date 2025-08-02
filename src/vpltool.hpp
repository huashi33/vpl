#ifndef VPL_TOOL_H
#define VPL_TOOL_H
#include <chrono>

namespace vpl{
class StopWatch{
    public:
        StopWatch(){}
        void start(){
            lastTimeStamp = std::chrono::high_resolution_clock::now();
        }
        void stop(const char* title){
            std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
            std::chrono::duration<int, std::ratio<1,1000000>> timeSpanus = std::chrono::duration_cast<std::chrono::duration<int, std::ratio<1,1000000>>>(now - lastTimeStamp);
            double ms = timeSpanus.count()  /1000.0;//ms
            printf("%s: %.3f ms\n",title,ms);
        }
        double stop(){
            std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
            std::chrono::duration<int, std::ratio<1,1000000>> timeSpanus = std::chrono::duration_cast<std::chrono::duration<int, std::ratio<1,1000000>>>(now - lastTimeStamp);
            double ms = timeSpanus.count()  /1000.0;//ms
            return ms;
        }
    private:
        std::chrono::high_resolution_clock::time_point lastTimeStamp;
};
}


#endif