# Lang-ship Hub Arduino API 仕様

## 1. 文書の目的

本文書は `lang-ship-hub-arduino` から利用する公開 HTTP API の仕様を定義する。

対象は ESP32 などの device であり、サーバー内部の管理画面用 API や内部実装は対象外とする。

## 2. 基本方針

- 通信は HTTP/HTTPS の JSON API を基本とする
- 接続先は `host`、`base path`、`https` 利用有無の組み合わせで設定できる
- `base path` は本番環境、開発環境、QA 環境、API version 用 path を含めて指定できる
- `base path` は末尾の `/` の有無に依存せず扱えること
- `http://` の利用は、利用者が明示的に `https` を使わない設定をした場合に扱うこと
- device は `workspace_id` を持って接続する
- device の通常認証は `device_id` と `secret_key` を用いる
- 自動登録時は `registration_token` を用いる
- 取得系 API は待機時間指定および server からの次回待機時間指示を扱える

## 3. 認証情報

### 3.1 Workspace 管理用

- `workspace_id`
- `workspace_token`

`workspace_token` は主にブラウザ管理画面で利用する。
device SDK は通常利用しない。

### 3.2 Device 自動登録用

- `workspace_id`
- `registration_token`

`registration_token` は device の初回登録時に利用する。

### 3.3 Device 通常接続用

- `workspace_id`
- `device_id`
- `secret_key`

device の通常通信はこの組み合わせで行う。

## 4. Device 状態

device は少なくとも以下の状態を持つ。

- `limited`
- `active`
- `disabled`

想定動作:

- `limited`
  telemetry、reported state、message 送受信、基本 API を利用できる
- `active`
  通常機能を利用できる
- `disabled`
  device 通信を拒否する

`command` と `OTA` は `active` device を対象とする。

## 5. 初期化と接続フロー

### 5.1 事前登録方式

1. 管理画面で device を追加する
2. `workspace_id`、`device_id`、`secret_key` を取得する
3. device に設定して接続する

### 5.2 自動登録方式

1. device は `workspace_id` と `registration_token` を持つ
2. device は初回登録 API を呼び出す
3. server は `device_id` と `secret_key` を払い出す
4. device は払い出された認証情報を保存する
5. 以後は通常接続用認証情報で通信する

## 6. 共通レスポンス方針

API レスポンスは JSON を基本とする。

代表的な共通項目:

- `ok`
- `error`
- `error_code`
- `server_time`
- `next_wait_sec`

`next_wait_sec` は次回接続までの推奨待機時間であり、device はこれを利用できる。

## 7. 共通エラー方針

代表的なエラー:

- 認証失敗
- 権限不足
- 入力不正
- 対象未存在
- device 状態不正

HTTP ステータスと JSON の両方で判定できることを前提とする。

## 8. API 一覧

### 8.1 Ping

`GET /api/ping`

用途:

- 設定した接続先の Lang-ship Hub へ到達できるか確認する
- Wi-Fi 接続後の HTTP/HTTPS 疎通確認に利用する
- `https` 利用設定に従って実行する

レスポンス例:

```json
{
  "ok": true,
  "service": "Lang-ship Hub",
  "time": "2026-04-15T12:00:00Z"
}
```

### 8.2 Device 自動登録

`POST /api/device/register`

用途:

- `registration_token` による初回登録

リクエスト例:

```json
{
  "workspace_id": "abcd1234",
  "registration_token": "reg-token",
  "device_name": "living-room-esp32",
  "device_type": "esp32",
  "mac_address": "AA:BB:CC:DD:EE:FF",
  "chip_id": "esp32-00112233",
  "firmware_version": "1.0.0"
}
```

レスポンス例:

```json
{
  "ok": true,
  "device_id": "12",
  "secret_key": "device-secret",
  "device_status": "limited",
  "next_wait_sec": 5
}
```

SDK 呼び出しイメージ:

```cpp
LangShipHub hub;

LangShipHubConfig config;
config.serverHost = "iot.lang-ship.com";
config.serverBasePath = "/v1/";
config.useHttps = true;
hub.begin(config);
hub.setWorkspaceId("abcd1234");
hub.setRegistrationToken("reg-token");

LangShipProvisionRequest req;
req.deviceName = "living-room-esp32";
req.deviceType = "esp32";
req.macAddress = "AA:BB:CC:DD:EE:FF";
req.chipId = "esp32-00112233";
req.firmwareVersion = "1.0.0";

LangShipProvisionResponse res;
if (hub.registerDevice(req, res)) {
  hub.setDeviceId(res.deviceId);
  hub.setSecretKey(res.secretKey);
}
```

### 8.3 Telemetry 送信

`POST /api/device/telemetry`

用途:

- device の時系列データ送信

リクエスト例:

```json
{
  "workspace_id": "abcd1234",
  "device_id": "12",
  "secret_key": "device-secret",
  "recorded_at": "2026-04-15T12:00:00Z",
  "payload": {
    "temperature": 24.5,
    "humidity": 60
  }
}
```

SDK 呼び出しイメージ:

```cpp
JsonDocument payload;
payload["temperature"] = 24.5;
payload["humidity"] = 60;

hub.sendTelemetry(payload);
```

### 8.4 Desired State 取得

`GET /api/device/state/desired`

用途:

- device が現在の desired state を取得する

クエリ例:

- `workspace_id`
- `device_id`
- `secret_key`

レスポンス例:

```json
{
  "ok": true,
  "desired": {
    "led": true,
    "interval_sec": 30
  },
  "desired_version": 3,
  "next_wait_sec": 5
}
```

SDK 呼び出しイメージ:

```cpp
JsonDocument desired;
int desiredVersion = 0;

if (hub.getDesiredState(desired, desiredVersion)) {
  bool led = desired["led"] | false;
  int intervalSec = desired["interval_sec"] | 60;
}
```

### 8.5 Reported State 更新

`PUT /api/device/state/reported`

用途:

- device の現在状態を server へ報告する

リクエスト例:

```json
{
  "workspace_id": "abcd1234",
  "device_id": "12",
  "secret_key": "device-secret",
  "reported": {
    "led": true,
    "interval_sec": 30
  }
}
```

SDK 呼び出しイメージ:

```cpp
JsonDocument reported;
reported["led"] = true;
reported["interval_sec"] = 30;

hub.updateReportedState(reported);
```

### 8.6 Command 取得

`GET /api/device/commands`

用途:

- device 向け pending command の取得

クエリ例:

- `workspace_id`
- `device_id`
- `secret_key`
- `wait_sec`

レスポンス例:

```json
{
  "ok": true,
  "commands": [
    {
      "command_id": "c-100",
      "command_name": "restart",
      "payload": {
        "delay_ms": 1000
      }
    }
  ],
  "next_wait_sec": 2
}
```

SDK 呼び出しイメージ:

```cpp
LangShipCommand command;

if (hub.getNextCommand(command, 10)) {
  if (command.name == "restart") {
    delay(command.payload["delay_ms"] | 0);
    ESP.restart();
  }
}
```

### 8.7 Command 結果報告

`PUT /api/device/commands/result`

用途:

- command 実行結果の報告

リクエスト例:

```json
{
  "workspace_id": "abcd1234",
  "device_id": "12",
  "secret_key": "device-secret",
  "command_id": "c-100",
  "status": "succeeded",
  "result": {
    "message": "ok"
  }
}
```

SDK 呼び出しイメージ:

```cpp
JsonDocument result;
result["message"] = "ok";

hub.completeCommand("c-100", "succeeded", result);
```

### 8.8 Message 送信

`POST /api/device/messages`

用途:

- path 向け message の送信

リクエスト例:

```json
{
  "workspace_id": "abcd1234",
  "device_id": "12",
  "secret_key": "device-secret",
  "path": "devices/chat/general",
  "payload": {
    "text": "hello"
  },
  "ttl_sec": 3600,
  "persistent": false
}
```

SDK 呼び出しイメージ:

```cpp
JsonDocument payload;
payload["text"] = "hello";

hub.publishMessage("devices/chat/general", payload, 3600, false);
```

### 8.9 Message 取得

`GET /api/device/messages`

用途:

- path に対する新着 message または直近履歴の取得
- device 向け取得は path 完全一致を基本とする

クエリ例:

- `workspace_id`
- `device_id`
- `secret_key`
- `path`
- `since`
- `limit`
- `wait_sec`

レスポンス例:

```json
{
  "ok": true,
  "messages": [
    {
      "message_id": "m-001",
      "path": "devices/chat/general",
      "created_at": "2026-04-15T12:00:10Z",
      "payload": {
        "text": "hello"
      }
    }
  ],
  "next_wait_sec": 5
}
```

SDK 呼び出しイメージ:

```cpp
LangShipMessageList messages;

if (hub.getMessages("devices/chat/general", "2026-04-15T12:00:00Z", 10, 5, messages)) {
  for (size_t i = 0; i < messages.size(); ++i) {
    const LangShipMessage& message = messages[i];
    Serial.println(message.payload["text"].as<const char*>());
  }
}
```

### 8.10 OTA 情報取得

`GET /api/device/ota`

用途:

- device 向け配布物の取得

クエリ例:

- `workspace_id`
- `device_id`
- `secret_key`

レスポンス例:

```json
{
  "ok": true,
  "items": [
    {
      "ota_id": "ota-001",
      "type": "firmware",
      "version": "1.2.0",
      "download_url": "https://iot.lang-ship.com/files/ota/fw.bin"
    }
  ],
  "next_wait_sec": 60
}
```

SDK 呼び出しイメージ:

```cpp
LangShipOtaList items;

if (hub.getOtaItems(items)) {
  for (size_t i = 0; i < items.size(); ++i) {
    Serial.println(items[i].downloadUrl);
  }
}
```

### 8.11 Device Log 送信

`POST /api/device/events`

用途:

- device から任意ログを event として送信する

リクエスト例:

```json
{
  "workspace_id": "abcd1234",
  "device_id": "12",
  "secret_key": "device-secret",
  "level": "warn",
  "message": "wifi reconnect",
  "detail": {
    "reason": "ap lost"
  }
}
```

SDK 呼び出しイメージ:

```cpp
JsonDocument detail;
detail["reason"] = "ap lost";

hub.sendLog("warn", "wifi reconnect", detail);
```

## 9. 待機時間制御

取得系 API は `wait_sec` を受け付けられる。

- client は希望待機時間を指定できる
- server は server 側上限や負荷状況を考慮して実際の待機時間を決定できる
- server は応答時に `next_wait_sec` を返却できる
- device は `next_wait_sec` を用いて次回接続間隔を調整できる

SDK 呼び出しイメージ:

```cpp
int nextWaitSec = 5;
LangShipCommand command;

if (hub.getNextCommand(command, nextWaitSec)) {
  nextWaitSec = hub.getNextWaitSec();
}

delay(nextWaitSec * 1000);
```

## 10. Messaging の使い分け

- 時系列データの保存やグラフ化が目的なら `telemetry`
- 継続設定や目標状態なら `desired / reported state`
- 一回限りの命令なら `command`
- 軽量な任意メッセージ共有なら `messages`

## 11. OTA の扱い

OTA はファームウェア更新だけでなく、以下の配布も含む。

- firmware
- software
- config file
- certificate
- related asset

## 12. 今後の言語方針

本ファイルは日本語版である。

仕様が固まり次第、英語版 `API.md` を整備する。
