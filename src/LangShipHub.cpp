#include "LangShipHub.h"

#include <HTTPClient.h>
#include <WiFiClient.h>

#if __has_include(<WiFiClientSecure.h>)
#include <WiFiClientSecure.h>
#endif

void LangShipHub::begin(const LangShipHubConfig &config)
{
    config_ = config;
    lastError_ = "";
    lastStatusCode_ = 0;
}

bool LangShipHub::ping(LangShipPingResponse &response)
{
    response = LangShipPingResponse();
    lastError_ = "";
    lastStatusCode_ = 0;

    if (config_.serverUrl.length() == 0)
    {
        lastError_ = "server_url_empty";
        return false;
    }

    const String url = buildUrl("/api/ping");
    if (!sendGetRequest(url, response))
    {
        return false;
    }

    if (response.httpStatus == 301 || response.httpStatus == 302 || response.httpStatus == 307 || response.httpStatus == 308)
    {
        lastError_ = "redirect_detected";
        return false;
    }

    if (response.httpStatus < 200 || response.httpStatus >= 300)
    {
        lastError_ = "unexpected_status";
        return false;
    }

    if (extractJsonString(response.rawBody, "service", response.service))
    {
        // no-op
    }
    if (extractJsonString(response.rawBody, "time", response.time))
    {
        // no-op
    }

    response.ok = response.rawBody.indexOf("\"ok\":true") >= 0 || response.rawBody.indexOf("\"ok\": true") >= 0;
    if (!response.ok)
    {
        lastError_ = "ping_response_invalid";
        return false;
    }

    return true;
}

bool LangShipHub::sendGetRequest(const String &url, LangShipPingResponse &response)
{
    if (isHttpsUrl(url))
    {
        return sendHttpsGetRequest(url, response);
    }

    return sendHttpGetRequest(url, response);
}

bool LangShipHub::sendHttpGetRequest(const String &url, LangShipPingResponse &response)
{
    WiFiClient client;
    HTTPClient http;

    if (!http.begin(client, url))
    {
        lastError_ = "http_begin_failed";
        return false;
    }

    return handleHttpResponse(http, response);
}

bool LangShipHub::sendHttpsGetRequest(const String &url, LangShipPingResponse &response)
{
#if __has_include(<WiFiClientSecure.h>)
    WiFiClientSecure client;
    HTTPClient http;

    if (config_.skipTlsVerify)
    {
        client.setInsecure();
    }

    if (!http.begin(client, url))
    {
        lastError_ = "http_begin_failed";
        return false;
    }

    return handleHttpResponse(http, response);
#else
    (void)url;
    (void)response;
    lastError_ = "https_not_supported";
    return false;
#endif
}

bool LangShipHub::handleHttpResponse(HTTPClient &http, LangShipPingResponse &response)
{
    const char *headerKeys[] = {"Location"};

    http.collectHeaders(headerKeys, 1);
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);

    const int httpCode = http.GET();
    lastStatusCode_ = httpCode;
    response.httpStatus = httpCode;

    if (httpCode <= 0)
    {
        lastError_ = http.errorToString(httpCode);
        http.end();
        return false;
    }

    response.rawBody = http.getString();
    response.location = http.header("Location");
    http.end();

    return true;
}

const LangShipHubConfig &LangShipHub::getConfig() const
{
    return config_;
}

const String &LangShipHub::getLastError() const
{
    return lastError_;
}

int LangShipHub::getLastStatusCode() const
{
    return lastStatusCode_;
}

String LangShipHub::buildUrl(const String &path) const
{
    String url = config_.serverUrl;

    if (!url.endsWith("/"))
    {
        url += "/";
    }

    if (path.startsWith("/"))
    {
        url += path.substring(1);
    }
    else
    {
        url += path;
    }

    return url;
}

bool LangShipHub::isHttpsUrl(const String &url) const
{
    return url.startsWith("https://");
}

bool LangShipHub::extractJsonString(const String &json, const String &key, String &value) const
{
    const String needle = "\"" + key + "\":";
    const int keyPos = json.indexOf(needle);
    if (keyPos < 0)
    {
        return false;
    }

    const int quoteStart = json.indexOf('"', keyPos + needle.length());
    if (quoteStart < 0)
    {
        return false;
    }

    const int quoteEnd = json.indexOf('"', quoteStart + 1);
    if (quoteEnd < 0)
    {
        return false;
    }

    value = json.substring(quoteStart + 1, quoteEnd);
    return true;
}
