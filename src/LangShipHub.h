#pragma once

#include <Arduino.h>

struct LangShipHubConfig
{
    String serverUrl;
    String workspaceId;
    String registrationToken;
    bool skipTlsVerify = false;
};

struct LangShipPingResponse
{
    bool ok = false;
    String service;
    String time;
    String rawBody;
    int httpStatus = 0;
};

class LangShipHub
{
public:
    LangShipHub() = default;

    void begin(const LangShipHubConfig &config);
    bool ping(LangShipPingResponse &response);

    const LangShipHubConfig &getConfig() const;
    const String &getLastError() const;
    int getLastStatusCode() const;

private:
    LangShipHubConfig config_;
    String lastError_;
    int lastStatusCode_ = 0;

    String buildUrl(const String &path) const;
    bool isHttpsUrl(const String &url) const;
    bool extractJsonString(const String &json, const String &key, String &value) const;
};
