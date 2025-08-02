#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "src/vplpuller.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


// namespace vpl{
//     class RingQueueAvFrame;
// } // namespace vpl

class RingQueueAvFrame;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


signals:
    // void imgUpdated(uint8_t *d,int);
public slots:
    void onClickPlay(bool flag);
    void onClickStop(bool flag);
private:
    Ui::MainWindow *ui;
    vpl::puller *pull;
    vpl::RqsiProvider mProvider;
    
    // vpl::RingQueueT<AVFrame *> rq;

};
#endif // MAINWINDOW_H
