#include "BSdk.h"
#include "nlohmann/json.hpp"
#include <QTimer>
#include <QDebug>
#include <string.h>
#include <iostream>

BSdk::BSdk(const std::string &accessKeyId, const std::string &accessKeySecret, const std::string &appId, const std::string &code, QObject *parent)
    : m_accessKeyId(accessKeyId),
      m_accessKeySecret(accessKeySecret),
      m_appId(appId),
      m_code(code),
      QObject{parent}
{
    bapi = new BApi(accessKeyId, accessKeySecret);
}

BSdk::~BSdk()
{
    if (bapi != nullptr)
    {
        delete bapi;
        bapi = nullptr;
    }
    if (danMuQWebsocket != nullptr)
    {
        delete danMuQWebsocket;
        danMuQWebsocket = nullptr;
    }
}

void BSdk::CreateWebsocket()
{
    if (danMuQWebsocket != nullptr)
    {
        delete danMuQWebsocket;
        danMuQWebsocket = nullptr;
    }
    danMuQWebsocket = new DanmaKuWebSocket(apiInfo);
    // 绑定消息接口
    connect(danMuQWebsocket, &DanmaKuWebSocket::SdkWebsocketReceiveed, this, &BSdk::WebsocketReceived);
    connect(danMuQWebsocket, &DanmaKuWebSocket::SdkWebsocketDisconnected, this, &BSdk::WebsocketDisconnected);
}

void BSdk::Start()
{
    // 同步版本
    std::string m_startResponse = bapi->StartInteractivePlay(m_code, m_appId);
    nlohmann::json jsonData = nlohmann::json::parse(m_startResponse);
    assert(jsonData["code"].get<int64_t>() == 0);
    timerId = startTimer(1000); // 掉线检测和发送心跳包
    apiInfo.setValue(jsonData["data"]);
    CreateWebsocket();

    // 异步版本
    /*
    bapi->StartInteractivePlayCallBack(
        m_code,
        m_appId,
        [&](const std::string &jsonText)
        {
            m_startResponse = jsonText;
            nlohmann::json jsonData = nlohmann::json::parse(m_startResponse);
            assert(jsonData["code"].get<int64_t>() == 0);
            timerId = startTimer(1000); // 掉线检测和发送心跳包
            apiInfo.setValue(jsonData["data"]);
            CreateWebsocket();
        });
        */
}

void BSdk::Stop()
{
    // Stop的话，还是使用同步吧
    std::string response = bapi->EndInteractivePlay(m_appId, apiInfo.gameId);
    nlohmann::json jsonData = nlohmann::json::parse(response);
    if (jsonData.find("code") != jsonData.end())
    {
        if (jsonData["code"].get<int64_t>() == 0)
        {
            qDebug() << QString::fromLocal8Bit("请求结束成功");
        }
        else
        {
            qDebug() << QString::fromLocal8Bit("请求结束失败");
        }
    }
    else
    {
        qDebug() << QString::fromLocal8Bit("请求错误");
    }

    qDebug() << response.c_str();
    killTimer(timerId);
}

// 计时器
void BSdk::timerEvent(QTimerEvent *event)
{
    static int count = 0;
    // 这里使用的是19秒发一次心跳包
    if (count >= 19)
    {
        // 同步版本
        // bapi->HeartBeatInteractivePlay(apiInfo.gameId);

        // 异步版本
        bapi->HeartBeatInteractivePlayCallBack(
            apiInfo.gameId,
            [this](const std::string &response)
            {
                nlohmann::json jsonData = nlohmann::json::parse(response);
                if (jsonData.find("code") != jsonData.end())
                {
                    if (jsonData["code"].get<int64_t>() == 0)
                    {
                        qDebug() << QString::fromLocal8Bit("发送心跳包响应成功");
                    }
                    else
                    {
                        qDebug() << QString::fromLocal8Bit("发送心跳包响应失败，有可能是超时了");

                        // 这样直接用我还没测试过，可能会有bug
                        this->Start();
                    }
                }
                else
                {
                    qDebug() << QString::fromLocal8Bit("发送心跳包失败");
                }
            });

        count = 0;
    }
    else
    {
        ++count;
    }
    // 每秒都会判断websocket是否断开了，断开的话就重连
    if (danMuQWebsocket == nullptr)
    {
        qDebug() << QString::fromUtf8("websocket re connect");
        CreateWebsocket();
    }
}

// Websocket收到数据
void BSdk::WebsocketReceived(const std::string &message)
{
    nlohmann::json jsonData = nlohmann::json::parse(message);
    std::string cmd = jsonData["cmd"].get<std::string>();

    qDebug() << QString::fromUtf8(message.c_str());

    // std::cout << std::endl
    //           << message << std::endl;

    if (cmd.find("LIVE_OPEN_PLATFORM_DM") != std::string::npos)
    {
        this->danmaData.setValue(jsonData["data"]);
        emit ReceivedDanmaKu(this->danmaData);
    }
    else if (cmd.find("LIVE_OPEN_PLATFORM_SEND_GIFT") != std::string::npos)
    {
        this->giftData.setValue(jsonData["data"]);
        emit ReceivedGift(this->giftData);
    }
    else if (cmd.find("LIVE_OPEN_PLATFORM_SUPER_CHAT") != std::string::npos)
    {
        this->superChatData.setValue(jsonData["data"]);
        emit ReceivedSuperChat(this->superChatData);
    }
    else if (cmd.find("LIVE_OPEN_PLATFORM_SUPER_CHAT_DEL") != std::string::npos)
    {
        this->superChatDelData.setValue(jsonData["data"]);
        emit ReceivedSuperChatDel(this->superChatDelData);
    }
    else if (cmd.find("LIVE_OPEN_PLATFORM_GUARD") != std::string::npos)
    {
        this->guardBuyData.setValue(jsonData["data"]);
        emit ReceivedGuardBuy(this->guardBuyData);
    }
}

void BSdk::WebsocketDisconnected()
{
    // 解绑
    disconnect(danMuQWebsocket, &DanmaKuWebSocket::SdkWebsocketReceiveed, this, &BSdk::WebsocketReceived);
    disconnect(danMuQWebsocket, &DanmaKuWebSocket::SdkWebsocketDisconnected, this, &BSdk::WebsocketDisconnected);

    // 删除，然后等待计时器重新连接
    delete danMuQWebsocket;
    danMuQWebsocket = nullptr;
}
