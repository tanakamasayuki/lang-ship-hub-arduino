#include <WiFi.h>
#include <LangShipHub.h>

// en: First target is ESP32.
// ja: 最初のターゲットは ESP32 です。
// en: This example only shows basic network connection and endpoint settings.
// ja: このサンプルではネットワーク接続と endpoint 設定のみを扱います。
// en: This example uses HTTPS without certificate verification for the first connectivity check.
// ja: このサンプルでは最初の疎通確認のために、証明書未検証の HTTPS を利用します。

#if __has_include("arduino_secrets.h")
#include "arduino_secrets.h"
#else
#define WIFI_SSID "YourSSID"
#define WIFI_PASS "YourPassword"
#define LANG_SHIP_SERVER_URL "https://iot.lang-ship.com/v1/"
#endif

// en: Keep the SDK instance global so it can also be used from loop() and helper functions.
// ja: loop() や補助関数からも使えるように SDK インスタンスはグローバルに保持します。
LangShipHub hub;

void setup()
{
    Serial.begin(115200);
    delay(1000);

    // en: Show example title on serial monitor.
    // ja: シリアルモニタにサンプル名を表示します。
    Serial.println();
    Serial.println("01_basic_connect");

    // en: Connect to Wi-Fi with the credentials defined in arduino_secrets.h.
    // ja: arduino_secrets.h で定義した認証情報を使って Wi-Fi に接続します。
    unsigned long start = millis();

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.print("Connecting to Wi-Fi");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");

        if (millis() - start > 30000)
        {
            Serial.println();
            Serial.println("Wi-Fi connect timeout");
            break;
        }
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println();
        Serial.println("Wi-Fi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        // en: Initialize the SDK with the endpoint setting.
        // ja: endpoint 設定を使って SDK を初期化します。
        LangShipHubConfig config;
        config.serverUrl = LANG_SHIP_SERVER_URL;

        // en: HTTPS is used for this example, but certificate verification is disabled.
        // ja: このサンプルでは HTTPS を使いますが、証明書検証は無効化しています。
        // en: Use this only for the first connectivity test when you do not have a certificate yet.
        // ja: まだ証明書を用意していない初回疎通確認でのみ使ってください。
        // en: Accessing other servers with TLS verification disabled is not secure.
        // ja: TLS 検証を無効にしたまま他のサーバーへアクセスするのは安全ではありません。
        config.skipTlsVerify = true;

        hub.begin(config);

        // en: Call the SDK ping function to confirm internet and Lang-ship Hub connectivity.
        // ja: SDK の ping 関数を呼び出して、インターネットと Lang-ship Hub への疎通を確認します。
        LangShipPingResponse ping;
        if (hub.ping(ping))
        {
            Serial.print("ping_service: ");
            Serial.println(ping.service);
            Serial.print("ping_time: ");
            Serial.println(ping.time);
        }
        else
        {
            Serial.println("Ping failed");
        }
    }

    // en: Print the basic Lang-ship Hub connection setting.
    // ja: Lang-ship Hub の基本接続設定を表示します。
    Serial.println("Lang-ship Hub basic configuration");
    Serial.print("server_url: ");
    Serial.println(LANG_SHIP_SERVER_URL);

    // en: This example stops here and prepares for the provisioning example.
    // ja: このサンプルはここで終了し、次の登録サンプルへ進む前提です。
    Serial.println("Ready for next step: device provisioning");
}

void loop()
{
    delay(1000);
}
