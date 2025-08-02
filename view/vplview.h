#ifndef VPLVIEW_H
#define VPLVIEW_H

#include <QWidget>
#include "src/vplringqueue.hpp"


namespace Ui {
class VplView;
}
struct SwsContext;

class VplView : public QWidget,public vpl::RqsiPopFrameHandler{
    Q_OBJECT

public:
    explicit VplView(QWidget *parent = nullptr);
    ~VplView();
    bool start();
    bool stop();
    void onHandle(AVFrame *frameSrc);

protected:
    void paintEvent(QPaintEvent *) override;
signals:
    void imgUpdated(AVFrame *);
public slots:
    void onImgUpdated(AVFrame *);


private:
private:
    Ui::VplView *ui;
    AVFrame *frameRgb;
    SwsContext *img_convert_ctx;

};

#endif // VPLVIEW_H
