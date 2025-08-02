#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <thread>
extern "C" {
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
}
#include "src/vplrqframe.hpp"
#include "src/vpldefine.h"



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    connect(ui->pushButton,&QPushButton::clicked,this,&MainWindow::onClickPlay);
    connect(ui->pushButton_2,&QPushButton::clicked,this,&MainWindow::onClickStop);


}



void MainWindow::onClickPlay(bool flag){
    QString url = ui->lineEdit->text();
    pull = new vpl::puller();
    pull->open(url.toUtf8().data());
    
    std::vector<vpl::RqsiPopFrameHandler  *> vecView = {
        ui->vplview0,ui->vplview1,ui->vplview2,ui->vplview3,ui->vplview4,
        // ui->vplviewgl0,ui->vplviewgl1,ui->vplviewgl2,ui->vplviewgl3,ui->vplviewgl4
    };

    VplFrameParam_t fp = pull->getFrameParam();
    for(int i = 0;i < vecView.size();++i){
        vpl::RingQueueSpscItem *rq = mProvider.add(&fp);
        pull->addRq(rq);
        vecView[i]->setRq(rq);
        vecView[i]->start();

    }

    pull->start();

    
    
}
void MainWindow::onClickStop(bool flag){
    if(pull){
        pull->stop();
        pull->close();
        pull->removeAllRq();
        delete pull;
        pull = nullptr;
    }



    
    std::vector<vpl::RqsiPopFrameHandler *> vecView = {
        ui->vplview0,ui->vplview1,ui->vplview2,ui->vplview3,ui->vplview4,
        // ui->vplviewgl0,ui->vplviewgl1,ui->vplviewgl2,ui->vplviewgl3,ui->vplviewgl4
    };
    for(int i = 0;i < vecView.size();++i){
        vecView[i]->stop();
    }

    
    mProvider.removeAll();
}

MainWindow::~MainWindow()
{
    delete ui;
}


