#include "BApi.h"
#include <QTimer>
#include <QNetworkAccessManager>
#include <QEventLoop>
#include <QNetworkReply>
#include <ctime>
#include <map>
#include <QCryptographicHash>
#include <QString>
#include <QUuid>
#include <QMessageAuthenticationCode>
#include <functional>

BApi::BApi(const std::string &accessKeyId, const std::string &accessKeySecret)
    : m_accessKeyId(accessKeyId),
      m_accessKeySecret(accessKeySecret)
{
}

// 添加验证用的请求头
void BApi::ApiSetReqHeader(QNetworkRequest &request, const std::string &bodyData)
{
    std::map<std::string, std::string> keyValuePair;

    // qt的uuid带了花括号，使用时要去掉！
    auto uuid = QUuid::createUuid().toString().remove("{").remove("}");

    // 注意了，此处顺序很重要，但凡你改变一下任意2条的顺序位置都会导致验证不通过!!!!!
    keyValuePair["x-bili-accesskeyid"] = m_accessKeyId;
    keyValuePair["x-bili-content-md5"] = QCryptographicHash::hash(bodyData.c_str(), QCryptographicHash::Md5).toHex().toStdString();
    keyValuePair["x-bili-signature-method"] = "HMAC-SHA256";
    keyValuePair["x-bili-signature-nonce"] = uuid.toStdString();
    keyValuePair["x-bili-signature-version"] = "1.0";
    keyValuePair["x-bili-timestamp"] = std::to_string(std::time(0));

    // 字符串拼接，末尾不能出现'\n'
    std::string sig = "";
    for (auto &pair : keyValuePair)
    {
        request.setRawHeader(pair.first.c_str(), pair.second.c_str());
        if (sig.length() == 0)
        {
            sig += pair.first + ":" + pair.second;
        }
        else
        {
            sig += "\n" + pair.first + ":" + pair.second;
        }
    }

    // 说真的，这里直接用accessKeySecret当盐我是没想到，这样不就直接暴露开发者的密钥了么...随便反编译就可用拿到了...
    std::string auth = QMessageAuthenticationCode::hash(sig.c_str(), m_accessKeySecret.c_str(), QCryptographicHash::Sha256).toHex().toStdString();

    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Authorization", auth.c_str());
}

std::string BApi::ApiPost(const std::string &url, const std::string &bodyData)
{
    QByteArray queryErrorResult;
    queryErrorResult.clear();
    //设置超时处理定时器
    QTimer timer;
    timer.setInterval(10000);  // 设置超时时间 10 秒
    timer.setSingleShot(true); // 单次触发

    QByteArray postData; // 上传的json内容
    postData.append(bodyData.c_str(), bodyData.length());

    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkRequest request;

    ApiSetReqHeader(request, bodyData); // 请求头签名

    request.setUrl(QUrl(url.c_str())); // 设置要发送的数据

    QNetworkReply *reply = manager->post(request, postData);
    QEventLoop eventLoop;
    connect(&timer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    connect(manager, &QNetworkAccessManager::finished, &eventLoop, &QEventLoop::quit);
    timer.start();    //启动定时器
    eventLoop.exec(); //启动事件循环

    if (timer.isActive()) //处理响应，定时器激活状态
    {
        timer.stop(); //停止定时器
        if (reply->error() != QNetworkReply::NoError)
        {
            // http请求出错，进行错误处理
            qDebug() << "http error : " << reply->errorString() << ", data : " << reply->readAll().toStdString().c_str();
            goto errorDeleteEnd;
        }
        else
        {
            // http - 响应状态码
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            // qDebug() << "服务器返回的Code : " << statusCode;
            QByteArray replyContent = reply->readAll();
            disconnect(&timer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
            disconnect(manager, &QNetworkAccessManager::finished, &eventLoop, &QEventLoop::quit);
            reply->deleteLater();
            manager->deleteLater();
            return replyContent.toStdString();
        }
    }
    else
    {
        //超时处理
        reply->abort();
        goto errorDeleteEnd;
    }
errorDeleteEnd:
    disconnect(&timer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    disconnect(manager, &QNetworkAccessManager::finished, &eventLoop, &QEventLoop::quit);
    reply->deleteLater();
    manager->deleteLater();
    return queryErrorResult.toStdString();
}

void BApi::ApiPostCallBack(const std::string &url, const std::string &bodyData, std::function<void(const std::string &)> callback)
{
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    connect(manager, &QNetworkAccessManager::finished,
            [=](QNetworkReply *reply)
            {
                if (reply->error() != QNetworkReply::NoError)
                {
                    if (callback != nullptr)
                    {
                        callback(reply->errorString().toStdString());
                    }
                    reply->deleteLater();
                    manager->deleteLater();
                    return;
                }
                if (callback != nullptr)
                {
                    auto reply_buffer = reply->readAll();
                    callback(std::string(reply_buffer.data()));
                }
                reply->deleteLater();
                manager->deleteLater();
            });

    QNetworkRequest request;
    ApiSetReqHeader(request, bodyData);
    request.setUrl(QUrl(url.c_str()));

    QByteArray postData;
    postData.append(bodyData.c_str(), bodyData.length());
    QNetworkReply *reply = manager->post(request, postData);
}