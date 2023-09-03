#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <vector>
#include <QLineEdit>

using namespace cv;
using namespace std;
extern Net_config yolo_nets[4];



//色彩饱和度
#define max2(a,b) (a>b?a:b)
#define max3(a,b,c) (a>b?max2(a,c):max2(b,c))
#define min2(a,b) (a<b?a:b)
#define min3(a,b,c) (a<b?min2(a,c):min2(b,c))

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle(tr("CQU图像视频处理小工具"));

    ui->tabWidget->setCurrentIndex(0);
    //设置背景
    QPalette pal =this->palette();
    pal.setBrush(QPalette::Background,QBrush(QPixmap(":/image/longbk.png")));
    setPalette(pal);

    img_index=0;  //保存图片

    //程序运行显示默认图片
    QImage image(":/image/show.jpg");

    ui->imageShow->setPixmap(QPixmap::fromImage(image).scaled(ui->imageShow->size(),
                                               Qt::KeepAspectRatio,Qt::SmoothTransformation));

    ui->videoShow->setPixmap(QPixmap::fromImage(image).scaled(ui->imageShow->size(),
                                               Qt::KeepAspectRatio,Qt::SmoothTransformation));

    //显示应用app
    this->setWindowIcon(QIcon(":/image/app.png"));

    //this->setWindowFlags(Qt::FramelessWindowHint);
    //该工具栏颜色，设置背景后突然变黑
    ui->toolBar->setStyleSheet("background-color:rgb(255,255,255);");//更改背景颜色

//    //设置旋转下拉菜单
    spinmenu = new QMenu();
    act_left = new QAction("左转",this);
    act_right = new QAction("右转",this);
    act_image = new QAction("左右镜像",this);
    act_flip = new QAction("上下翻转",this);
    spinmenu->addAction(act_left); //把action追加到menu
    spinmenu->addAction(act_right);
    spinmenu->addAction(act_image);
    spinmenu->addAction(act_flip);


    //错误提醒
    messages = new Message(this);
    messages->SetDuration(1000);  // 1s钟后自动消失

    ui->spinaction->setMenu(spinmenu); //把menu加载到button
    connect(act_left,SIGNAL(triggered()),this,SLOT(onAct1Clicked_left()));
    connect(act_right,SIGNAL(triggered()),this,SLOT(onAct2Clicked_right()));
    connect(act_image,SIGNAL(triggered()),this,SLOT(onAct3Clicked_image()));
    connect(act_flip,SIGNAL(triggered()),this,SLOT(onAct4Clicked_flip()));

    //video

    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    connect(&timer, SIGNAL(timeout()), this, SLOT(updatePosition()));
    ui->stopBtn->setStyleSheet("border-radius:32px;"
                                             "background-image: url(:/image/stop.png);border:none;") ;
}

MainWindow::~MainWindow()
{
    delete ui;
}




/**
 * @brief adaptimageShow   图片尺寸大于label尺寸，初始显示调整为与label尺寸一致；小与label尺寸，显示图片本身尺寸
 * @param img 显示的图片
 * @retval 无
 */
void MainWindow::adaptimageShow(QImage img)
{
    int fitWidth = img.width() <= ui->imageShow->width() ? img.width() : ui->imageShow->width();
    int fitHeight = img.height() <= ui->imageShow->height() ?  img.height() : ui->imageShow->height();
    ui->imageShow->setPixmap(QPixmap::fromImage(img.scaled(fitWidth, fitHeight)));

    // QLabel缩放时维持图像宽高比例不变,以便放大和缩小
    //ui->imageShow->setPixmap(QPixmap::fromImage(img).scaled(ui->imageShow->size(),
                                               //Qt::KeepAspectRatio,Qt::SmoothTransformation));
}



/**
 * @brief on_selectpicBtn_clicked  打开目录，选择图片
 * @param img 显示的图片
 * @retval 无
 */
void MainWindow::on_selectpicBtn_clicked()
{
    imagePath = QFileDialog::getExistingDirectory(this, "打开文件", "../");
    if (imagePath.isEmpty())
    {    // 用户选择打开文件夹后又取消
        return;
    }
    //qDebug()<<imagePath;
    QDir imageDir(imagePath);
    QStringList filter;
    filter<<"*.jpg"<<"*.png"<<"*.bmp"<<"*.jpeg"<<"*.ppm"<<
            "*.JPG"<<"*.PNG"<<"*.JPEG";
    imageDir.setNameFilters(filter);

    //在打开的路径中得到文件名
    QStringList imageFileList = imageDir.entryList(filter);
    //qDebug()<<"\n"<<imageFileList;
    for(int i = 0; i < imageFileList.size(); i++)
    {
        //得到每一个图片的绝对路径
        QString imageFullDir = imagePath + "/" + imageFileList.at(i);
        imageNumber == 0 ? fullPathOfFiles.insert(i, imageFullDir)
                       : fullPathOfFiles.insert(imageNumber + i, imageFullDir);

        QListWidgetItem *imageItem=new QListWidgetItem;
        imageItem->setIcon(QIcon(imageFullDir));
        imageItem->setText(imageFullDir);
        ui->imageList->addItem(imageItem);
    }
    ui->imageList->setCurrentRow(imageNumber);
    imageNumber += imageFileList.size();
    imageLoad(fullPathOfFiles.at(0));

    picIndex = ui->imageList->currentRow()+1; //为了获取原图，为颜色操作做准备
    showImage(picIndex);

}


/**
 * @brief imageLoad  将新打开的文件夹中第一张图片显示在Qlabel中
 * @param fillAllPath 图片路径
 * @retval 无
 */
void MainWindow::imageLoad(QString fillAllPath)
{
    QImage imgTemp;
    fileDirOpen = true; //  判断是否有图片文件载入
    imgTemp.load(fillAllPath);
    zoomImg.load(fillAllPath);
    if(imgTemp.isNull()){return;}
    adaptimageShow(imgTemp);
    picImportIndex = imageNumber;   // 再次打开文件夹或拖拽导入时更新首张显示
}





/**
 * @brief on_upimageToolBtn_clicked  //上一张
 * @param 无
 * @retval 无
 */
void MainWindow::on_upimageToolBtn_clicked()
{
    if (false == fileDirOpen)
    {
        messages->Push(MessageType::MESSAGE_TYPE_SUCCESS, QString("                           提示\n                   请先选择您需要打开的图片文件"));
        return;
    }

    picIndex = ui->imageList->currentRow();
    --picIndex;
    if (picIndex < 0)
    {
        messages->Push(MessageType::MESSAGE_TYPE_SUCCESS, QString("                           提示\n                   当前浏览的是列表中第一张图片，将从列表中最后一张重新开始"));
        picIndex = fullPathOfFiles.size() - 1;
    }

    showImage(picIndex);
}


/**
 * @brief on_downimageToolBtn_clicked  //下一张
 * @param 无a
 * @retval 无
 */
void MainWindow::on_downimageToolBtn_clicked()
{
    if (false == fileDirOpen)
    {
        messages->Push(MessageType::MESSAGE_TYPE_SUCCESS, QString("                           提示\n                   请先选择您需要打开的图片文件"));
        return;
    }

    picIndex = ui->imageList->currentRow();
    picIndex++;
    if (picIndex > fullPathOfFiles.size() - 1)
    {
        messages->Push(MessageType::MESSAGE_TYPE_SUCCESS, QString("                           提示\n                   已浏览至最后一张图片，再次点击将从第一张重新开始"));
        picIndex = 0;
    }
    showImage(picIndex);
}


/**
 * @brief showImage  //点击上一张，下一张按钮时加载图片
 * @param index 图片列表下标
 * @retval 无
 */
void MainWindow::showImage(int index)
{
    QImage img;
    QString imageName = fullPathOfFiles.at(index);
    zoomImg.load(imageName);
    img.load(imageName);
    if(img.isNull()){return;}
    adaptimageShow(img);
    ui->imageList->setCurrentRow(index);
}


/**
 * @brief on_imageList_currentRowChanged  //点击QListWidget中不同图片缩略图,Label中显示缩略图对应的大图
 * @param currentRow 当前选中的图片
 * @retval 无
 */
void MainWindow::on_imageList_currentRowChanged(int currentRow)
{
    if (currentRow == -1)
    {
        return;
    }
    QImage currentImg;
    QString currentPic = ui->imageList->item(currentRow)->text();
    zoomImg.load(currentPic);
    currentImg.load(currentPic);
    if(currentImg.isNull()){return;}
    adaptimageShow(currentImg);
}



/**
 * @brief QCVMat2QImage 图片Mat转换成QImage
 * @param mat 要转换的图片
 * @retval 无
 */
QImage QCVMat2QImage(const cv::Mat& mat)
{
    if(!mat.empty())
    {
        const unsigned char* data = mat.data;           //Mat的数据
        int width = mat.cols;                           //读取图片的列数
        int height = mat.rows;                          //读取图片的行数
        int bytesPerLine = static_cast<int>(mat.step);  //读取Mat的第一行
        switch(mat.type())                              //读取Mat的类型,补全4字节对齐
        {
            //8 bit , ARGB
            case CV_8UC4:
            {
                QImage image(data, width, height, bytesPerLine, QImage::Format_ARGB32);
                return image;
            }
            //8 bit BGR
            case CV_8UC3:
            {
                //QImage image(data, width, height, bytesPerLine, QImage::Format_RGB888);
                QImage image(data, width, height, mat.step, QImage::Format_RGB888);
                //QImage image(data, width, height, width*3, QImage::Format_RGB888);
                return image.rgbSwapped();
                //return image.copy();
            }
            case CV_8UC1:
            {
                QImage image(data, width, height, bytesPerLine, QImage::Format_Grayscale8);
                return image;
            }
            default:
            {
                qDebug()<<"image format error\n";
                return QImage();
            }
        }
    }
}

/**
 * @brief QImage2Mat 图片QImage转换成Mat
 * @param image 要转换的图片
 * @retval 无
 */
Mat QImage2Mat(QImage image)
{
    Mat cvMat;
    //qDebug()<<"QImage2Mat :"<<image.format();
    switch(image.format())
    {
        case QImage::Format_ARGB32:
        case QImage::Format_RGB32:
        case QImage::Format_ARGB32_Premultiplied:
            cvMat = cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.bits(), image.bytesPerLine());
            break;
        case QImage::Format_RGB888:
            cvMat = cv::Mat(image.height(), image.width(), CV_8UC3, (void*)image.bits(), image.bytesPerLine());
            cv::cvtColor(cvMat, cvMat, COLOR_BGR2RGB);
            break;
        case QImage::Format_Indexed8:
            cvMat = cv::Mat(image.height(), image.width(), CV_8UC1, (void*)image.bits(), image.bytesPerLine());
            break;
        case QImage::Format_Grayscale8:
            cvMat = cv::Mat(image.height(), image.width(), CV_8UC1, (void*)image.bits(), image.bytesPerLine());
            break;
        default:
        {
            qDebug()<<"image format error\n";
        }
    }
    return cvMat;
}


/**
 * @brief getCVimage  //从路径中得到cv::Mat类型图片
 * @param 无
 * @retval 无
 */
cv::Mat MainWindow::getCVimage()
{
    //获取图片
    cv::Mat cv_image;
    if (imagePath.isEmpty())
    {
        messages->Push(MessageType::MESSAGE_TYPE_ERROR, QString("                           错误\n                   请先打开图片"));
    }
    else
    {
        picIndex = ui->imageList->currentRow();
        QString imageName = fullPathOfFiles.at(picIndex);
        string path = imageName.toStdString();
        cv_image = cv::imread(path);
    }
    return cv_image;
}

/**
 * @brief on_saveaction_triggered  //保存图片
 * @param 无
 * @retval 无
 */
void MainWindow::on_saveaction_triggered()
{
    QString save_file = QString("/home/xiaochao/cqu/nn/cv_task1/save/%1.jpg").arg(img_index);
    const QPixmap *qimage = ui->imageShow->pixmap();
    cv::Mat cv_image = QImage2Mat(qimage->toImage());
    cv::imwrite(save_file.toStdString(),cv_image);
    img_index += 1;

}



/**
 * @brief on_grayBtn_clicked  //灰度化
 * @param 无
 * @retval 无
 */
void MainWindow::on_grayBtn_clicked()
{
    cv::Mat cv_image =  getCVimage();
    if(cv_image.empty())
    {

    }
    else
    {
        cv::Mat cv_image_gray;
        cvtColor(cv_image, cv_image_gray, COLOR_BGR2GRAY);  //灰度化
        QImage qimage_gray = QCVMat2QImage(cv_image_gray);
        adaptimageShow(qimage_gray);
    }
}


/**
 * @brief on_objBtn_clicked  目标检测
 * @param 无
 * @retval 无
 */
void MainWindow::on_objBtn_clicked()
{
    if (imagePath.isEmpty())
    {
        messages->Push(MessageType::MESSAGE_TYPE_ERROR, QString("                           错误\n                   请先打开图片"));
    }
    else
    {
        MyYolo yolo_model(yolo_nets[3]);
        picIndex = ui->imageList->currentRow();
        QString imageName = fullPathOfFiles.at(picIndex);
        string path = imageName.toStdString();
        Mat cv_image = cv::imread(path);
        yolo_model.detect(cv_image);

        QImage qimage = QCVMat2QImage(cv_image);
        adaptimageShow(qimage);
    }
}


/**
 * @brief on_notBtn_clicked  颜色反转
 * @param 无
 * @retval 无
 */
void MainWindow::on_notBtn_clicked()
{
    cv::Mat cv_image =  getCVimage();
    if(cv_image.empty())
    {

    }
    else
    {
        cv::Mat not_img,gray_img;
        cv::cvtColor(cv_image, gray_img, COLOR_BGR2GRAY);
        cv::bitwise_not(gray_img,not_img);
        QImage qimage_resize = QCVMat2QImage(not_img);
        adaptimageShow(qimage_resize);
    }
}


/**
 * @brief on_originBtn_clicked  原图显示
 * @param 无
 * @retval 无
 */
void MainWindow::on_originBtn_clicked()
{
    if (imagePath.isEmpty())
    {
        messages->Push(MessageType::MESSAGE_TYPE_ERROR, QString("                           错误\n                   请先打开图片"));
    }
    else
    {
        picIndex = ui->imageList->currentRow();
        showImage(picIndex);
    }

}

/**
 * @brief on_horizontalSlider_R_valueChanged  调节R通道颜色
 * @param value 调节值的大小
 * @retval 无
 */
void MainWindow::on_horizontalSlider_R_valueChanged(int value)
{
    if (imagePath.isEmpty())
    {
        //为了居中对齐
        messages->Push(MessageType::MESSAGE_TYPE_ERROR, QString("                           错误\n                   请先打开图片"));
    }
    else
    {
        cv::Mat cv_image =  getCVimage();
        int height = cv_image.rows; //row表示行，rows表示行的总数，即图像的高
        int width = cv_image.cols; //col表示列，cols表示列的总数，即图像的宽
        for(int i=0;i<height;i++)
        {
            for(int j=0;j<width;j++)
            {

                cv_image.at<Vec3b>(i, j)[2] = value;

            }
        }
        QImage images = QCVMat2QImage(cv_image);
        adaptimageShow(images);
    }
}



/**
 * @brief on_horizontalSlider_G_valueChanged  调节G通道颜色
 * @param value 调节值的大小
 * @retval 无
 */
void MainWindow::on_horizontalSlider_G_valueChanged(int value)
{
    if (imagePath.isEmpty())
    {
        //为了居中对齐
        messages->Push(MessageType::MESSAGE_TYPE_ERROR, QString("                           错误\n                   请先打开图片"));
    }
    else
    {
        cv::Mat cv_image =  getCVimage();
        int height = cv_image.rows; //row表示行，rows表示行的总数，即图像的高
        int width = cv_image.cols; //col表示列，cols表示列的总数，即图像的宽
        for(int i=0;i<height;i++)
        {
            for(int j=0;j<width;j++)
            {

                cv_image.at<Vec3b>(i, j)[1] = value;

            }
        }
        QImage images = QCVMat2QImage(cv_image);
        adaptimageShow(images);

    }
}



/**
 * @brief on_horizontalSlider_G_valueChanged  调节B通道颜色
 * @param value 调节值的大小
 * @retval 无
 */
void MainWindow::on_horizontalSlider_B_valueChanged(int value)
{
    if(imagePath.isEmpty())
    {
        //为了居中对齐
        messages->Push(MessageType::MESSAGE_TYPE_ERROR, QString("                           错误\n                   请先打开图片"));
    }
    else
    {

        cv::Mat cv_image =  getCVimage();
        int height = cv_image.rows; //row表示行，rows表示行的总数，即图像的高
        int width = cv_image.cols; //col表示列，cols表示列的总数，即图像的宽
        for(int i=0;i<height;i++)
        {
            for(int j=0;j<width;j++)
            {

                cv_image.at<Vec3b>(i, j)[0] = value;

            }
        }
        QImage images = QCVMat2QImage(cv_image);
        adaptimageShow(images);

    }
}


/**
 * @brief on_exitaction_triggered  退出程序
 * @param 无
 * @retval 无
 */
void MainWindow::on_exitaction_triggered()
{
    this->close();
}

/**
 * @brief on_spinaction_triggered  图片旋转菜单栏弹出
 * @param 无
 * @retval 无
 */
void MainWindow::on_spinaction_triggered()
{
    spinmenu->exec(QPoint(QCursor::pos().x(),QCursor::pos().y())); //打开menu
}



/**
 * @brief onAct1Clicked_left  左旋
 * @param 无
 * @retval 无
 */
void MainWindow::onAct1Clicked_left()
{

#if 0
    if(imagePath.isEmpty())
    {
        //为了居中对齐
        messages->Push(MessageType::MESSAGE_TYPE_ERROR, QString("                           错误\n                   请先打开图片"));
    }
    else
    {
        cv::Mat cv_image =  getCVimage();
        cv::Mat left_img;
        transpose(cv_image, left_img);
        QImage qimage = QCVMat2QImage(left_img);
        adaptimageShow(qimage);
    }
#endif
    if(imagePath.isEmpty())
    {
        //为了居中对齐
        messages->Push(MessageType::MESSAGE_TYPE_ERROR, QString("                           错误\n                   请先打开图片"));
    }
    else
    {
        QImage images(ui->imageShow->pixmap()->toImage());
        QMatrix matrix;
        matrix.rotate(-90.0);//逆时针旋转90度
        images= images.transformed(matrix,Qt::FastTransformation);
        ui->imageShow->setPixmap(QPixmap::fromImage(images));
        ui->imageShow->setAlignment(Qt::AlignCenter);
    }
}


/**
 * @brief onAct2Clicked_right  右旋
 * @param 无
 * @retval 无
 */
void MainWindow::onAct2Clicked_right()
{
#if 0
    cv::Mat cv_image =  getCVimage();
    cv::imshow("src",cv_image);
    cv::Mat flip_img;
    cv::flip(cv_image, flip_img,1); //此函数是绕y轴旋转180， 也就是关于y轴对称
    cv::imshow("cv",flip_img);
    QImage qimage = QCVMat2QImage(flip_img);
    adaptimageShow(qimage);
#endif
    if(imagePath.isEmpty())
    {
        //为了居中对齐
        messages->Push(MessageType::MESSAGE_TYPE_ERROR, QString("                           错误\n                   请先打开图片"));
    }
    else
    {
        QImage images(ui->imageShow->pixmap()->toImage());
        QMatrix matrix;
        matrix.rotate(90.0);//逆时针旋转90度
        images= images.transformed(matrix,Qt::FastTransformation);
        ui->imageShow->setPixmap(QPixmap::fromImage(images));
        ui->imageShow->setAlignment(Qt::AlignCenter);
    }
}


/**
 * @brief onAct3Clicked_image  左右镜像
 * @param 无
 * @retval 无
 */
void MainWindow::onAct3Clicked_image()
{
    if(imagePath.isEmpty())
    {
        //为了居中对齐
        messages->Push(MessageType::MESSAGE_TYPE_ERROR, QString("                           错误\n                   请先打开图片"));
    }
    else
    {
        QImage images(ui->imageShow->pixmap()->toImage());
        images = images.mirrored(true, false);
        ui->imageShow->setPixmap(QPixmap::fromImage(images));
        ui->imageShow->setAlignment(Qt::AlignCenter);
    }
}


/**
 * @brief onAct4Clicked_flip  上下翻转
 * @param 无
 * @retval 无
 */
void MainWindow::onAct4Clicked_flip()
{
#if 0
    cv::Mat cv_image =  getCVimage();
    cv::imshow("src",cv_image);
    cv::Mat flip_img;
    cv::flip(cv_image, flip_img,0);
    cv::imshow("cv",flip_img);
    QImage qimage = QCVMat2QImage(flip_img);
    adaptimageShow(qimage);
#endif
    if(imagePath.isEmpty())
    {
        //为了居中对齐
        messages->Push(MessageType::MESSAGE_TYPE_ERROR, QString("                           错误\n                   请先打开图片"));
    }
    else
    {
        if(ui->imageShow->pixmap()!=nullptr)
        {
            QImage images(ui->imageShow->pixmap()->toImage());
            images = images.mirrored(false, true);
            ui->imageShow->setPixmap(QPixmap::fromImage(images));
            ui->imageShow->setAlignment(Qt::AlignCenter);
        }
        else
        {
            messages->Push(MessageType::MESSAGE_TYPE_ERROR, QString("                           错误\n                   请先打开图片"));
        }
    }
}



/**
 * @brief on_horizontalSlider_valueChanged  调节图片亮度
 * @param value 亮度值的大小
 * @retval 无
 */
void MainWindow::on_horizontalSlider_valueChanged(int value)
{
    if(imagePath.isEmpty())
    {
        //为了居中对齐
        messages->Push(MessageType::MESSAGE_TYPE_ERROR, QString("                           错误\n                   请先打开图片"));
    }
    else
    {
        cv::Mat dst;
        cv::Mat cv_image =  getCVimage();
        //对比度跟亮度的调整
        int height = cv_image.rows;
        int width = cv_image.cols;
        dst = Mat::zeros(cv_image.size(), cv_image.type());
        float alpha = 1.5;
        float beta = value;

        Mat m1;
        cv_image.convertTo(m1, CV_32F);
        for (int row = 0; row < height; row++)
        {
            for (int col = 0; col < width; col++)
            {
                if (cv_image.channels() == 3)
                {
                    int b = m1.at<Vec3f>(row, col)[0];
                    int g = m1.at<Vec3f>(row, col)[1];
                    int r = m1.at<Vec3f>(row, col)[2];

                    dst.at<Vec3b>(row, col)[0] = saturate_cast<uchar>(b * alpha + beta);
                    dst.at<Vec3b>(row, col)[1] = saturate_cast<uchar>(g * alpha + beta);
                    dst.at<Vec3b>(row, col)[2] = saturate_cast<uchar>(r * alpha + beta);

                }
                else if (cv_image.channels() == 1)
                {
                    float v = cv_image.at<uchar>(row, col);
                    dst.at<uchar>(row, col) = saturate_cast<uchar>(v * alpha + beta);
                }
            }
        }
        QImage images = QCVMat2QImage(dst);
        adaptimageShow(images);
        ui->label_light->setText(QString::number(value));
    }
}



/**
 * @brief on_horizontalSlider_erzhi_valueChanged  二值化
 * @param value 图像的阈值
 * @retval 无
 */
void MainWindow::on_horizontalSlider_erzhi_valueChanged(int value)
{
    if(imagePath.isEmpty())
    {
        //为了居中对齐
        messages->Push(MessageType::MESSAGE_TYPE_ERROR, QString("                           错误\n                   请先打开图片"));
    }
    else
    {
        int index = ui->imageList->currentRow();
        QString imageName = fullPathOfFiles.at(index);
        QImage image(imageName);
        cv::Mat cv_image = QImage2Mat(image);
        cv::Mat gray_image,th_image;
        cvtColor(cv_image, gray_image, COLOR_BGR2GRAY);  //灰度化
        threshold(gray_image,th_image,value,255,THRESH_BINARY);
        QImage images = QCVMat2QImage(th_image);
        adaptimageShow(images);
        ui->label_yuzhi->setText(QString::number(value));
    }

}



/**
 * @brief on_horizontalSlider_duibi_valueChanged  调节对比度
 * @param value 对比度的大小
 * @retval 无
 */
void MainWindow::on_horizontalSlider_duibi_valueChanged(int value)
{
    if(imagePath.isEmpty())
    {
        //为了居中对齐
        messages->Push(MessageType::MESSAGE_TYPE_ERROR, QString("                           错误\n                   请先打开图片"));
    }
    else
    {
        cv::Mat dst;
        cv::Mat cv_image =  getCVimage();
        //对比度跟亮度的调整
        int height = cv_image.rows;
        int width = cv_image.cols;
        dst = Mat::zeros(cv_image.size(), cv_image.type());
        float alpha = value;
        float beta = 10;

        Mat m1;
        cv_image.convertTo(m1, CV_32F);
        for (int row = 0; row < height; row++)
        {
            for (int col = 0; col < width; col++)
            {
                if (cv_image.channels() == 3)
                {
                    int b = m1.at<Vec3f>(row, col)[0];
                    int g = m1.at<Vec3f>(row, col)[1];
                    int r = m1.at<Vec3f>(row, col)[2];

                    dst.at<Vec3b>(row, col)[0] = saturate_cast<uchar>(b * alpha + beta);
                    dst.at<Vec3b>(row, col)[1] = saturate_cast<uchar>(g * alpha + beta);
                    dst.at<Vec3b>(row, col)[2] = saturate_cast<uchar>(r * alpha + beta);

                }
                else if (cv_image.channels() == 1)
                {
                    float v = cv_image.at<uchar>(row, col);
                    dst.at<uchar>(row, col) = saturate_cast<uchar>(v * alpha + beta);
                }
            }
        }
        QImage images = QCVMat2QImage(dst);
        adaptimageShow(images);
        ui->label_light->setText(QString::number(value));
    }

}



/**
 * @brief on_horizontalSlider_duibi_valueChanged  调节饱和度
 * @param src 原图
 * @param percent 饱和度值
 * @retval 无
 */
cv::Mat MainWindow::Saturation(cv::Mat src, int percent)
{
    float Increment = percent* 1.0f / 100;
    cv::Mat temp = src.clone();
    int row = src.rows;
    int col = src.cols;
    for (int i = 0; i < row; ++i)
    {
        uchar *t = temp.ptr<uchar>(i);
        uchar *s = src.ptr<uchar>(i);
        for (int j = 0; j < col; ++j)
        {
            uchar b = s[3 * j];
            uchar g = s[3 * j + 1];
            uchar r = s[3 * j + 2];
            float max = max3(r, g, b);
            float min = min3(r, g, b);
            float delta, value;
            float L, S, alpha;
            delta = (max - min) / 255;
            if (delta == 0)
                continue;
            value = (max + min) / 255;
            L = value / 2;
            if (L < 0.5)
                S = delta / value;
            else
                S = delta / (2 - value);
            if (Increment >= 0)
            {
                if ((Increment + S) >= 1)
                    alpha = S;
                else
                    alpha = 1 - Increment;
                alpha = 1 / alpha - 1;
                t[3 * j + 2] =static_cast<uchar>( r + (r - L * 255) * alpha);
                t[3 * j + 1] = static_cast<uchar>(g + (g - L * 255) * alpha);
                t[3 * j] = static_cast<uchar>(b + (b - L * 255) * alpha);
            }
            else
            {
                alpha = Increment;
                t[3 * j + 2] = static_cast<uchar>(L * 255 + (r - L * 255) * (1 + alpha));
                t[3 * j + 1] = static_cast<uchar>(L * 255 + (g - L * 255) * (1 + alpha));
                t[3 * j] = static_cast<uchar>(L * 255 + (b - L * 255) * (1 + alpha));
            }
        }
    }
    return temp;
}



/**
 * @brief on_horizontalSlider_duibi_valueChanged  调节饱和度
 * @param value 饱和度值
 * @retval 无
 */
void MainWindow::on_horizontalSlider_baohe_valueChanged(int value)
{
    if(imagePath.isEmpty())
    {
        //为了居中对齐
        messages->Push(MessageType::MESSAGE_TYPE_ERROR, QString("                           错误\n                   请先打开图片"));
    }
    else
    {
        cv::Mat cv_image =  getCVimage();
        cv::Mat saturation_image = Saturation(cv_image,value);
        QImage images=QCVMat2QImage(saturation_image);
        adaptimageShow(images);
    }
}



/**
 * @brief on_erodeBtn_clicked  图像腐蚀
 * @param 无
 * @retval 无
 */
void MainWindow::on_erodeBtn_clicked()
{
    cv::Mat cv_image =  getCVimage();
    if(cv_image.empty())
    {

    }
    else
    {
        cvtColor(cv_image, cv_image, COLOR_BGR2GRAY);
        //高斯滤波
        GaussianBlur(cv_image, cv_image, Size(3, 3),0, 0, BORDER_DEFAULT);
        //Canny检测
        int edgeThresh =100;
        Canny(cv_image, cv_image, edgeThresh, edgeThresh * 3, 3);
        QImage qimage = QCVMat2QImage(cv_image);
        adaptimageShow(qimage);
    }
}


/**
 * @brief on_dilateBtn_clicked  图像膨胀
 * @param 无
 * @retval 无
 */
void MainWindow::on_dilateBtn_clicked()
{
    cv::Mat cv_image =  getCVimage();
    if(cv_image.empty())
    {

    }
    else
    {
        cv::Mat dilate_image;  //存放腐蚀后的图像
        Mat struct1= getStructuringElement(1, Size(3, 3));  //十字结构元素
        cv::dilate(cv_image, dilate_image, struct1);
        QImage qimage = QCVMat2QImage(dilate_image);
        adaptimageShow(qimage);
    }
}


/**
 * @brief on_bigaction_triggered  图像放大
 * @param 无
 * @retval 无
 */
void MainWindow::on_bigaction_triggered()
{
    cv::Mat cv_image =  getCVimage();
    if(cv_image.empty())
    {

    }
    else
    {
        cv::Mat big_image;
        int height = cv_image.rows;
        int width = cv_image.cols;
        Size dsize = Size(height*2, width*2);
        cv::resize(cv_image, big_image, dsize, 0, 0, INTER_AREA);
        QImage qimage = QCVMat2QImage(big_image);
        adaptimageShow(qimage);
    }
}


/**
 * @brief on_smallaction_triggered  图像缩小
 * @param 无
 * @retval 无
 */
void MainWindow::on_smallaction_triggered()
{
    cv::Mat cv_image =  getCVimage();
    if(cv_image.empty())
    {

    }
    else
    {
        cv::Mat small_image;
        int height = cv_image.rows;
        int width = cv_image.cols;
        //缩小后的尺寸
        Size dsize = Size(height/13, width/13);
        cv::resize(cv_image, small_image, dsize, 0, 0, INTER_AREA);
        cv::imshow("q",small_image);
        QImage qimage = QCVMat2QImage(small_image);
        adaptimageShow(qimage);
    }
}


/**
 * @brief on_changeBtn_clicked  图像仿射变换
 * @param 无
 * @retval 无
 */
void MainWindow::on_changeBtn_clicked()
{
    cv::Mat cv_image =  getCVimage();
    if(cv_image.empty())
    {

    }
    else
    {
        Mat transform_img;
        Point2f srcPoints1[4], dstPoints1[4]; //投影（透视视角）变换
        srcPoints1[0] = Point2f(0, 0);//左上/原点
        srcPoints1[1] = Point2f(0, cv_image.rows);//左下角
        srcPoints1[2] = Point2f(cv_image.cols, 0);//右上角
        srcPoints1[3] = Point2f(cv_image.cols, cv_image.rows);//右下角
        //前  4点到后4点变换
        dstPoints1[0] = Point2f(0 + 200, 0 + 200);
        dstPoints1[1] = Point2f(0 + 100, cv_image.rows);
        dstPoints1[2] = Point2f(cv_image.cols, 0 + 100);
        dstPoints1[3] = Point2f(cv_image.cols, cv_image.rows);
        //获取设计好的仿射矩阵
        Mat M2 = getAffineTransform(srcPoints1, dstPoints1);
        warpAffine(cv_image, transform_img, M2, cv_image.size(), 1, 0);
        QImage qimage = QCVMat2QImage(transform_img);
        adaptimageShow(qimage);

    }
}




//vodei 处理

//选择视频
void MainWindow::on_selectvideoBtn_clicked()
{
    video_path = QFileDialog::getOpenFileName(this,tr("选择视频"),tr("Video (*.WMV *.mp4 *.rmvb *.flv)"));
    //video_path = QFileDialog::getOpenFileName(this,tr("选择视频"),"../");
    if(video_path!=nullptr)
    {
        qDebug()<<1<<video_path;
        //打开视频文件：其实就是建立一个VideoCapture结构
        capture.open(video_path.toStdString());
        //检测是否正常打开:成功打开时，isOpened返回ture
        if (!capture.isOpened())
        {
             QMessageBox::warning(nullptr, "提示", "打开视频失败！", QMessageBox::Yes |  QMessageBox::Yes);
        }
        else
        {
            //获取整个帧数
            long totalFrameNumber = capture.get(CAP_PROP_FRAME_COUNT);

            //设置开始帧()
            long frameToStart = 0;
            capture.set(CAP_PROP_POS_FRAMES, frameToStart);
            //ui->textEdit->append(QString::fromLocal8Bit("from %1 frame").arg(frameToStart));

            //获取帧率
            double rate = capture.get(CAP_PROP_FPS);
            //ui->textEdit->append(QString::fromLocal8Bit("Frame rate: %1 ").arg(rate));

            delay = 1000 / rate;
            timer.start(delay);
            type=0;
            isstart =!isstart;

            ItemModel = new QStandardItemModel(this);
            QStringList strList; // 需要展示的数据
            strList.append(video_path);
            int nCount = strList.size();
            for(int i = 0; i < nCount; i++)
            {
                QString string = static_cast<QString>(strList.at(i));
                QStandardItem *item = new QStandardItem(string);
                ItemModel->appendRow(item);
            }
            ui->listView->setModel(ItemModel); // listview设置Model
        }
    }
}


//进度条随视频移动
void MainWindow::updatePosition()
{
    long totalFrameNumber = capture.get(CAP_PROP_FRAME_COUNT);
    ui->videohorizontalSlider->setMaximum(totalFrameNumber);
    long frame=capture.get(CAP_PROP_POS_FRAMES );
    ui->videohorizontalSlider->setValue(frame);
}




//秒转分函数
QString MainWindow::stom(int s)
{
    QString m;
    if(s/60==0)
    {
        m=QString::number (s%60);
    }
    else
    {
        m=QString::number (s/60)+":"+QString::number (s%60);
    }
    return m;
}



//timer触发函数
void MainWindow::onTimeout()
{
    Mat frame;
    //读取下一帧
    double rate = capture.get(CAP_PROP_FPS);
    double nowframe=capture.get(CAP_PROP_POS_FRAMES );
    int nows=nowframe/rate;
    long totalFrameNumber = capture.get(CAP_PROP_FRAME_COUNT);
    int totals=totalFrameNumber/rate;
    ui->label_3->setText(stom(nows)+"/"+stom(totals));

    if (!capture.read(frame))
    {
        return;
    }

    // 视频处理
    if(type==1)  //灰度
    {
        cvtColor(frame,frame,COLOR_BGR2GRAY);
    }
    else if(type==2)  //边缘检测
    {
        cvtColor(frame, frame, COLOR_BGR2GRAY);
        //高斯滤波
        GaussianBlur(frame, frame, Size(3, 3),0, 0, BORDER_DEFAULT);
        //Canny检测
        int edgeThresh =100;

        Canny(frame, frame, edgeThresh, edgeThresh * 3, 3);
    }
    else if(type==3) //高斯滤波
    {
         GaussianBlur(frame, frame, Size(3, 3), 0, 0);
    }
    else if(type==4) //二值化
    {
        cvtColor(frame,frame,COLOR_BGR2GRAY);
        threshold(frame, frame, 96, 255, THRESH_BINARY);
    }
    else if(type == 5) //目标检测
    {
        MyYolo yolo_model(yolo_nets[3]);
        yolo_model.detect(frame);
    }

    QImage image=QCVMat2QImage(frame);

    QSize qs = ui->videoShow->rect().size();
    ui->videoShow->setPixmap(QPixmap::fromImage(image).scaled(qs));
    ui->videoShow->setAlignment(Qt::AlignCenter);
    ui->videoShow->repaint();
}

//原视频播放
void MainWindow::on_videoBtn_clicked()
{
    type = 0;
}

//灰度
void MainWindow::on_vgrayBtn_clicked()
{
    type = 1;
}

//边缘检测
void MainWindow::on_meanBtn_clicked()
{
    type = 2;
}

//高斯滤波
void MainWindow::on_smothBtn_clicked()
{
    type = 3;
}

//二值化
void MainWindow::on_verzhiBtn_clicked()
{
    type = 4;
}

void MainWindow::on_vobjBtn_clicked()
{
    type = 5;
}


//暂停与开始
void MainWindow::on_stopBtn_clicked()
{
    if(video_path!=nullptr)
    {
        if(isstart)
        {
            timer.stop();
            isstart=false;
            ui->stopBtn->setStyleSheet("border-radius:32px;"
                                                     "background-image: url(:/image/start.png);border:none;") ;
        }
        else
        {
            timer.start(delay);
            isstart=true;
            ui->stopBtn->setStyleSheet("border-radius:32px;"
                                                     "background-image: url(:/image/stop.png);border:none;") ;
        }
    }
}
