#include "loadingdlg.h"
#include "ui_loadingdlg.h"
#include <QLabel>
#include <QMovie>
#include <QGraphicsOpacityEffect>
#include <QGuiApplication>
#include <QScreen>

LoadingDlg::LoadingDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoadingDlg)
{
    ui->setupUi(this);

    // 设置对话框的窗口标志：使其为标准对话框、无边框、支持系统菜单，并始终保持在最上层
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground); // 设置背景透明
    // 获取屏幕尺寸
    setFixedSize(parent->size()); // 设置对话框为全屏尺寸

    QMovie *movie = new QMovie(":/res/loading.gif"); // 加载动画的资源文件
    ui->loading_lb->setMovie(movie);
    movie->start();
}

LoadingDlg::~LoadingDlg()
{
    delete ui;
}
