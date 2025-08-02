cmake_minimum_required(VERSION 3.5)

project(Myproj LANGUAGES CXX C)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# set(CMAKE_BUILD_TYPE "Release")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/3rd/include)
#src
set(VIEW_SRC    mainwindow.cpp
                mainwindow.h
                mainwindow.ui
                view/vplview.cpp
                view/vplview.h
                view/vplview.ui
                view/vplviewgl.cpp
                view/vplviewgl.h
                )
set(PROJECT_SOURCES
        main.cpp
        # demux_decode.c
        # mux.c
        src/vplpuller.cpp
        ${VIEW_SRC}
)

set(QT_PATH "C:/Qt/Qt5.12.6/5.12.6/msvc2017_64")
set(CMAKE_PREFIX_PATH ${QT_PATH}/lib/cmake)
# libs
link_directories(${CMAKE_SOURCE_DIR}/3rd/lib/win64)
set(FFMPEG_LIBS avcodec avdevice avfilter avformat avutil swresample swscale)
set(OPENCV_LIBS opencv_world460)
set(OPENVINO_LIBS openvino)


find_package(Qt5 COMPONENTS Core Gui Widgets OpenGL REQUIRED)
add_executable(vpl ${PROJECT_SOURCES})
target_link_libraries(vpl PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets Qt5::OpenGL ${FFMPEG_LIBS} ${OPENCV_LIBS} ${OPENVINO_LIBS})
