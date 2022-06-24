#ifndef BSDK_H
#define BSDK_H

#include <QObject>
#include "BApi.h"
#include "ApiInfo.h"
#include "DanmaKuWebSocket.h"
#include "memory"

class BSdk : public QObject
{
    Q_OBJECT

private:
    std::string m_accessKeyId;     // 开发者id
    std::string m_accessKeySecret; // 开发者密钥
    std::string m_appId;           // 项目id
    std::string m_code;            // 主播身份码

    int timerId; // 计时器id

    ApiInfo apiInfo;

    DanmaData danmaData;               // 弹幕
    GiftData giftData;                 // 礼物
    GuardBuyData guardBuyData;         // 大航海
    SuperChatData superChatData;       // 付费留言
    SuperChatDelData superChatDelData; // 过时的付费留言id

    DanmaKuWebSocket *danMuQWebsocket = nullptr;
    BApi *bapi = nullptr;

private:
    void WebsocketReceived(const std::string &message);
    void WebsocketDisconnected();
    void CreateWebsocket();

public:
    explicit BSdk(const std::string &accessKeyId, const std::string &accessKeySecret, const std::string &appId, const std::string &code, QObject *parent = nullptr);
    ~BSdk();
    void Start(); // 开启
    void Stop();  // 结束
    virtual void timerEvent(QTimerEvent *event) override;

    // 这些是信号，要自行绑定
signals:
    void ReceivedDanmaKu(const DanmaData &danMu);
    void ReceivedGift(const GiftData &gift);
    void ReceivedGuardBuy(const GuardBuyData &guardBuy);
    void ReceivedSuperChat(const SuperChatData &superChat);
    void ReceivedSuperChatDel(const SuperChatDelData &superChatDel);
};

#endif // BSDK_H
