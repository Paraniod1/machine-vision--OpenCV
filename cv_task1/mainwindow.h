#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QFileDialog>
#include <QDebug>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <math.h>
#include <QMenu>
#include <message.h>
#include <myyolo.h>
#include <QStringListModel>
#include <QStandardItemModel>
#include <QModelIndex>

using namespace cv;


Mat QImage2Mat(QImage image);
QImage QCVMat2QImage(const cv::Mat& mat);

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public:
    int type=0;//视频操作类型

private slots:
    void on_selectpicBtn_clicked();

    void on_upimageToolBtn_clicked();

    void on_downimageToolBtn_clicked();

    void on_imageList_currentRowChanged(int currentRow);

    void on_grayBtn_clicked();

    void on_notBtn_clicked();

    void on_horizontalSlider_R_valueChanged(int value);

    void on_horizontalSlider_G_valueChanged(int value);

    void on_horizontalSlider_B_valueChanged(int value);

    void on_exitaction_triggered();

    void on_spinaction_triggered();

    void onAct1Clicked_left();

    void onAct2Clicked_right();

    void onAct3Clicked_image();

    void onAct4Clicked_flip();

    void on_horizontalSlider_valueChanged(int value);

    void on_originBtn_clicked();

    void on_horizontalSlider_erzhi_valueChanged(int value);

    void on_horizontalSlider_duibi_valueChanged(int value);

    void on_horizontalSlider_baohe_valueChanged(int value);

    void on_erodeBtn_clicked();

    void on_dilateBtn_clicked();

    void on_bigaction_triggered();

    void on_smallaction_triggered();

    void on_selectvideoBtn_clicked();

    void on_videoBtn_clicked();

    void onTimeout();

    void updatePosition();


    void on_vgrayBtn_clicked();

    void on_verzhiBtn_clicked();

    void on_meanBtn_clicked();

    void on_smothBtn_clicked();

    void on_changeBtn_clicked();

    void on_stopBtn_clicked();

    void on_objBtn_clicked();

    void on_vobjBtn_clicked();


    void on_saveaction_triggered();

private:
    QString imagePath;
    QStringList fullPathOfFiles;

    int imageNumber;
    bool fileDirOpen;
    QImage zoomImg;

    int picIndex;   // 用于图片切换
    int picImportIndex = 0;


    //下拉菜单
    QAction* act_left;
    QAction* act_right;
    QAction* act_image;
    QAction* act_flip;
    QMenu *spinmenu;

    //错误警告
    Message* messages;


    //video
    QString videoSrcDir;//视频路径
    VideoCapture capture; //用来读取视频结构
    QTimer timer;//视频播放的定时器
    int beishu;//调节播放速率
    int delay;//帧延迟时间
    bool isstart=false;
    QString video_path;

    QStringListModel *Model;
    QStandardItemModel *ItemModel;



    //用于图片显示
    void imageLoad(QString fillAllPath);
    void showImage(int index);
    void adaptimageShow(QImage img);
    void listWidgetRowChanged(int currentRow);
    cv::Mat getCVimage();
    cv::Mat Saturation(cv::Mat src, int percent);
    QString stom(int s);
    int img_index;

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
