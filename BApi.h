#ifndef BAPI_H
#define BAPI_H

#include <QObject>
#include <string>
#include <QNetworkRequest>
#include <functional>

class BApi : public QObject
{
    Q_OBJECT
private:
    const std::string OpenLiveDomain = "https://live-open.biliapi.com";
    const std::string RoomIdApi = "/v1/common/roomIdInfo";
    const std::string InteractivePlayStart = "/v2/app/start";
    const std::string InteractivePlayEnd = "/v2/app/end";
    const std::string InteractivePlayHeartBeat = "/v2/app/heartbeat";
    const std::string InteractivePlayBatchHeartBeat = "/v2/app/batchHeartbeat";

private:
    std::string m_accessKeyId;
    std::string m_accessKeySecret;

private:
    std::string ApiPost(const std::string &url, const std::string &bodyData);
    void ApiPostCallBack(const std::string &url, const std::string &bodyData, std::function<void(const std::string &)> callback);
    void ApiSetReqHeader(QNetworkRequest &request, const std::string &bodyData);

public:
    BApi(const std::string &accessKeyId, const std::string &accessKeySecret);

    inline std::string StartInteractivePlay(const std::string &code, const std::string &appId)
    {
        return ApiPost((OpenLiveDomain + InteractivePlayStart), (std::string("{\"code\":\"") + code + "\",\"app_id\":" + appId + "}"));
    }

    inline std::string HeartBeatInteractivePlay(const std::string &gameId)
    {
        return ApiPost((OpenLiveDomain + InteractivePlayHeartBeat), (std::string("{\"game_id\":\"") + gameId + "\"}"));
    }

    inline std::string EndInteractivePlay(const std::string &appId, const std::string &gameId)
    {
        return ApiPost((OpenLiveDomain + InteractivePlayEnd), (std::string("{\"app_id\":") + appId + ",\"game_id\":\"" + gameId + "\"}"));
    }

    inline void StartInteractivePlayCallBack(const std::string &code, const std::string &appId, std::function<void(const std::string &)> callback = nullptr)
    {
        ApiPostCallBack((OpenLiveDomain + InteractivePlayStart), (std::string("{\"code\":\"") + code + "\",\"app_id\":" + appId + "}"), (callback));
    }

    inline void HeartBeatInteractivePlayCallBack(const std::string &gameId, std::function<void(const std::string &)> callback = nullptr)
    {
        ApiPostCallBack((OpenLiveDomain + InteractivePlayHeartBeat), (std::string("{\"game_id\":\"") + gameId + "\"}"), (callback));
    }

    inline void EndInteractivePlayCallBack(const std::string &appId, const std::string &gameId, std::function<void(const std::string &)> callback = nullptr)
    {
        ApiPostCallBack((OpenLiveDomain + InteractivePlayEnd), (std::string("{\"app_id\":") + appId + ",\"game_id\":\"" + gameId + "\"}"), (callback));
    }
};

#endif // BAPI_H
