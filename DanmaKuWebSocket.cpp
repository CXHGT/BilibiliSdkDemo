/**********************************************
 * 这个websocket连接是通过我之前的野生版本改装而成
 *
 * 而Qt野生版本是通过之前的cocos2dx版本改装的，
 *
 * 所以某些部分的编写或许不规范，不喜勿喷！
 *
 * 本来想好好写一下注释的，但奈何记忆力有限，某些部
 * 分已经忘记是干啥用的了，所以注释我已经尽力了。
 **********************************************/

#include "DanmaKuWebSocket.h"
#include <QUrl>
#include <iostream>
#include <zlib.h>
#include <thread>
#include "nlohmann/json.hpp"

DanmaKuWebSocket::DanmaKuWebSocket(const ApiInfo &apiInfo) : m_apiInfo(apiInfo)
{
    this->websocket = new QWebSocket();
    // 连接时触发
    connect(this->websocket, &QWebSocket::connected, this, &DanmaKuWebSocket::WebSocketConnected);
    // 断开时触发
    connect(this->websocket, &QWebSocket::disconnected, this, &DanmaKuWebSocket::WebSocketDisconnected);
    // 接收到数据
    connect(this->websocket, &QWebSocket::binaryMessageReceived, this, &DanmaKuWebSocket::WebSocketReceived);
    // 默认就使用第一条url吧
    this->websocket->open(QUrl(apiInfo.wssLink[0].c_str()));

    std::cout << "create bilibili sdk websocket " << std::endl;
}

DanmaKuWebSocket::~DanmaKuWebSocket()
{
    // 销毁得使用deleteLater，不能用delete
    this->websocket->deleteLater();
}

void DanmaKuWebSocket::WebSocketConnected()
{
    std::stringstream ss;
    danmakuByte buffer[4];

    //写入验证字符串长度
    getBytesByInt(buffer, (int)(m_apiInfo.authBody.length() + 16));

    ss << buffer[0] << buffer[1] << buffer[2] << buffer[3];
    // 16是必须的，虽然不知道为什么
    getBytesByShort(buffer, (short)16);
    ss << buffer[0] << buffer[1];

    //有内鬼说已经有 protover = 3 了, 总之把protover 2用到死为止
    getBytesByShort(buffer, (short)2);
    ss << buffer[0] << buffer[1];

    // 7是加入房间的获取弹幕，2是心跳包防止强制掉线
    getBytesByInt(buffer, (int)7);
    ss << buffer[0] << buffer[1] << buffer[2] << buffer[3];

    getBytesByInt(buffer, (int)1);
    ss << buffer[0] << buffer[1] << buffer[2] << buffer[3];

    //最后将json字符串也写入进去
    ss << m_apiInfo.authBody;
    //最后生成房间包，并将包发送给服务器
    std::string roomPack = ss.str();

    // 此处的'\0'不能包括进去
    this->websocket->sendBinaryMessage(QByteArray(roomPack.c_str(), roomPack.length()));

    if (m_beatTimer != nullptr)
    {
        delete m_beatTimer;
    }
    m_beatTimer = new QTimer(this);
    // 定时发送心跳包,虽然官方说的是30秒一个，但是以防万一，提前1s发
    m_beatTimer->setInterval(29000);

    connect(m_beatTimer, &QTimer::timeout, [this]
            {
        //心跳包
        static char heartBeat[16] = {0x00, 0x00, 0x00, 0x10, 
                                      0x00, 0x10, 
                                      0x00, 0x01,
                                      0x00, 0x00, 0x00, 0x02, 
                                      0x00, 0x00, 0x00, 0x01};

        // 只有websocket还处于正常连接状态下才能发送心跳包
        if(this->websocket != nullptr && this->websocket->state() == QAbstractSocket::ConnectedState)
        {
            this->websocket->sendBinaryMessage(QByteArray(heartBeat,sizeof(heartBeat)));
        } });

    m_beatTimer->start();
}

void DanmaKuWebSocket::WebSocketDisconnected()
{
    // 断开连接了，那么就关闭websocket，并发送连接已经断开的信号
    this->websocket->close();
    emit SdkWebsocketDisconnected();
}

void DanmaKuWebSocket::WebSocketReceived(const QByteArray &message)
{
    //连接成功，返回26个字节的数据,这里直接省略这步的解析
    if (message.length() != 26)
    {
        danmakuByte *buffer = (danmakuByte *)message.data();
        //读取包的长度
        int packetLength = toInt(buffer, 0);

        int headerLength = toShort(buffer, 4);
        //处理类型,2为gzip数据包，弹幕，礼物，进入直播间的都是这种数据
        int version = toShort(buffer, 6);
        int action = toInt(buffer, 8);
        int parameter = toInt(buffer, 12);

        if (packetLength > 20)
        {
            // version == 2 表示数据使用了zlib压缩
            if (version == 2)
            {
                uLong unzip_size = UNZIP_BUFF_SIZE;
                if (uncompress(unzipBuffer, &unzip_size, buffer + 16, packetLength - 16) == 0)
                {
                    //此处，解压出来的所有数据被称之为大包，里面的每条弹幕信息被称为子包
                    //有时候是多个子包放在一起发送过来的，要对整个包解剖处理，初始位置肯定从0开始了
                    int allsize = 0;
                    //当要处理位置跟解压出来的大小一致时，那就是整个大包都处理完了
                    while (allsize < unzip_size)
                    {
                        //读取子包的长度，并记录
                        int packetLength2 = toInt(unzipBuffer, allsize + 0);
                        int headerLength2 = toShort(unzipBuffer, allsize + 4);
                        int version2 = toShort(unzipBuffer, allsize + 6);
                        int action2 = toInt(unzipBuffer, allsize + 8);
                        int parameter2 = toInt(unzipBuffer, allsize + 12);

                        //提取字符串
                        int end = packetLength2 + allsize;
                        char c = unzipBuffer[end];
                        unzipBuffer[end] = '\0';
                        std::string data = (char *)(unzipBuffer + allsize + 16);
                        unzipBuffer[end] = c;

                        // 然后就是处理数据了 ...
                        emit SdkWebsocketReceiveed(data);

                        //处理完这个子包后记得将大包处理位置进行偏移
                        allsize += packetLength2;
                    }
                }
            }
            else
            {
                memcpy(unzipBuffer, buffer + 16, packetLength - 16);
                unzipBuffer[packetLength - 16] = '\0';
                emit SdkWebsocketReceiveed((const char *)unzipBuffer);
            }
        }
    }
}

void DanmaKuWebSocket::getBytesByInt(danmakuByte *buffer, int value)
{
    buffer[0] = (danmakuByte)(value >> 24);
    buffer[1] = (danmakuByte)(value >> 16);
    buffer[2] = (danmakuByte)(value >> 8);
    buffer[3] = (danmakuByte)value;
}
void DanmaKuWebSocket::getBytesByShort(danmakuByte *buffer, short value)
{
    buffer[0] = (danmakuByte)(value >> 8);
    buffer[1] = (danmakuByte)value;
}
int DanmaKuWebSocket::toInt(danmakuByte *buffer, int index)
{
    return (((int)buffer[index] << 24) | ((int)buffer[index + 1] << 16) | ((int)buffer[index + 2] << 8) | (int)buffer[index + 3]);
}
int DanmaKuWebSocket::toShort(danmakuByte *buffer, int index)
{
    return ((int)buffer[index] << 8) | (int)buffer[index + 1];
}
