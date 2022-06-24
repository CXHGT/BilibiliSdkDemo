#ifndef __DANMAKU_QWEBSOCKET_H__
#define __DANMAKU_QWEBSOCKET_H__

#include <QObject>
#include <QTcpSocket>
#include <QNetworkRequest>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <memory>
#include <QtWebSockets/QWebSocket>
#include <QTimer>
#include "ApiInfo.h"

#define UNZIP_BUFF_SIZE 65536

typedef unsigned char danmakuByte;

class DanmaKuWebSocket : public QObject
{
    Q_OBJECT
private:
    danmakuByte unzipBuffer[UNZIP_BUFF_SIZE];

    const ApiInfo &m_apiInfo;

    QWebSocket *websocket = nullptr;
    QTimer *m_beatTimer = nullptr;

public:
    explicit DanmaKuWebSocket(const ApiInfo &apiInfo);
    ~DanmaKuWebSocket();

    void WebSocketConnected();
    void WebSocketDisconnected();
    void WebSocketReceived(const QByteArray &message);

    void getBytesByInt(danmakuByte *buffer, int value);
    void getBytesByShort(danmakuByte *buffer, short value);
    int toInt(danmakuByte *buffer, int index);
    int toShort(danmakuByte *buffer, int index);

signals:
    void SdkWebsocketReceiveed(const std::string &message);
    void SdkWebsocketDisconnected();
};

#endif // DANMUQWEBSOCKET_H
