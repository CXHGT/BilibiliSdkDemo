#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "BApi.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "nlohmann/json.hpp"
#include <QDebug>

MainWindow::MainWindow(const std::string &code, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    bsdk = new BSdk("accessKeyId", "accessKeySecret", "项目id", code);
    bsdk->Start();

    connect(bsdk, &BSdk::ReceivedDanmaKu, [](const DanmaData &danMu)
            { qDebug() << "get danmu : " << danMu.uname.c_str() << " : " << danMu.msg.c_str(); });
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    bsdk->Stop();
    event->accept();
}

MainWindow::~MainWindow()
{
    delete bsdk;
    delete ui;
}
