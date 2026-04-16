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
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.print("Connecting to Wi-Fi");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("Wi-Fi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // en: Create SDK config from library defaults, then apply optional local overrides.
    // ja: SDK 設定はライブラリの既定値から作成し、必要ならローカル設定で上書きします。
    LangShipHubConfig config;

    // en: HTTPS is used for this example, but certificate verification is disabled.
    // ja: このサンプルでは HTTPS を使いますが、証明書検証は無効化しています。
    // en: Use this only for the first connectivity test when you do not have a certificate yet.
    // ja: まだ証明書を用意していない初回疎通確認でのみ使ってください。
    // en: Accessing other servers with TLS verification disabled is not secure.
    // ja: TLS 検証を無効にしたまま他のサーバーへアクセスするのは安全ではありません。
    config.skipTlsVerify = true;

    hub.begin(config);

    // en: Print the basic Lang-ship Hub connection setting.
    // ja: Lang-ship Hub の基本接続設定を表示します。
    const LangShipHubConfig &currentConfig = hub.getConfig();
    Serial.println("Lang-ship Hub basic configuration");
    Serial.print("server_host: ");
    Serial.println(currentConfig.serverHost);
    Serial.print("server_base_path: ");
    Serial.println(currentConfig.serverBasePath);
    Serial.print("use_https: ");
    Serial.println(currentConfig.useHttps ? "true" : "false");
    Serial.print("skip_tls_verify: ");
    Serial.println(currentConfig.skipTlsVerify ? "true" : "false");

    // en: Ping is executed in loop() at a fixed interval.
    // ja: ping は loop() で一定間隔ごとに実行します。
    Serial.println("Ready to start periodic ping");
}

void loop()
{
    static unsigned long lastPingMillis = 0;
    const unsigned long PING_INTERVAL_MS = 10000;

    if (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        return;
    }

    if (millis() - lastPingMillis < PING_INTERVAL_MS)
    {
        delay(100);
        return;
    }

    lastPingMillis = millis();

    // en: Call the SDK ping function periodically to confirm internet and server connectivity.
    // ja: SDK の ping 関数を定期的に呼び出して、インターネットとサーバーへの疎通を確認します。
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
        Serial.print("ping_error: ");
        Serial.println(hub.getLastError());
        Serial.print("ping_status: ");
        Serial.println(ping.httpStatus);
        Serial.print("ping_location: ");
        Serial.println(ping.location);
        Serial.println("ping_response:");
        Serial.println(ping.rawBody);
    }
}
