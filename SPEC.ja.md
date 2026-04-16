# Lang-ship Hub Arduino SDK 仕様

## 1. 目的

本文書は `lang-ship-hub-arduino` リポジトリで公開する SDK 仕様を定義する。

SDK は ESP32 などのデバイスから `Lang-ship Hub` に接続するための Arduino ライブラリを提供する。ここでは、デバイス実装者に公開してよいサービス概念、クライアント側のデータモデル、HTTP ベースの連携モデルを定義する。

本 SDK は ESP32 をファーストターゲットとして設計し、他の Arduino 対応ボードへ展開しやすい構造とする。

## 2. 対象範囲

本文書では以下を扱う。

- SDK の責務
- クライアントから見えるサービス概念
- SDK が扱う認証情報
- device から見た接続モデル
- SDK が提供する主要機能

詳細な HTTP API、endpoint、request / response、payload 例は `API.ja.md` で定義する。
本文書ではサーバー内部実装は定義しない。

## 3. サービス概念

SDK は以下の概念を中心に構成する。

- workspace
- device
- telemetry
- device state
- command
- message
- OTA

`device state` は `desired state` と `reported state` を含む状態同期の概念として扱う。

## 4. SDK の責務

SDK は以下を提供する。

- endpoint 設定
- 接続先 environment ごとの endpoint 切り替え
- workspace と device の認証情報設定
- 自動登録用認証情報の設定
- API 呼び出し時の device 認証
- telemetry 送信
- device state の desired state 取得
- device state の reported state 更新
- command 取得
- command result 更新
- message publish
- message receive
- OTA metadata 取得

SDK は低レベルな HTTP 処理を隠蔽しつつ、プラットフォームの概念は利用者に見える形を維持する。

機種依存のある処理は、通信、保存、識別情報取得などの共通ロジックから分離し、他ボード対応時に置き換えやすい構造とする。

## 5. SDK が扱う認証情報

### 5.1 Workspace 管理用

- `workspace_id`
- `workspace_token`

`workspace_token` は主に管理画面で利用する。
SDK は通常利用しない。

### 5.2 Device 自動登録用

- `workspace_id`
- `registration_token`

`registration_token` は device の初回登録時に利用する。

### 5.3 Device 通常接続用

- `workspace_id`
- `device_id`
- `secret_key`

device の通常通信はこの組み合わせで行う。

## 6. Endpoint 設定方針

- SDK は `host`、`base path`、`https` 利用有無の組み合わせで endpoint を構成できること
- SDK は必要に応じて `http://` および `https://` を含む完全な endpoint 文字列も扱えること
- 既定および推奨の接続先は `https://` とすること
- `http://` の利用は、利用者が `https` を使わない設定を明示した場合に扱えること
- SDK は domain だけでなく base path を含む接続先を扱えること
- SDK は base path の末尾 `/` の有無に依存せず接続先を扱えること
- SDK は本番環境、開発環境、QA 環境など複数 environment の切り替えを行えること
- API version 切り替えのために、`/v1/` のような path を含む endpoint を扱えること
- QA 用の別 domain や staging 用の別 endpoint を指定できること

## 7. Device 接続モデル

### 6.1 Device 状態

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

### 6.2 事前登録方式

1. 管理画面で device を追加する
2. `workspace_id`、`device_id`、`secret_key` を取得する
3. device に設定して接続する

### 6.3 自動登録方式

1. device は `workspace_id` と `registration_token` を持つ
2. device は初回登録 API を呼び出す
3. server は `device_id` と `secret_key` を払い出す
4. device は払い出された認証情報を保存する
5. 以後は通常接続用認証情報で通信する

## 8. SDK が提供する機能

### 7.1 Telemetry

- 任意の JSON telemetry を送信できること
- telemetry は時系列データ送信に利用すること
- telemetry はセンサー値、状態値、計測値などの履歴保存とグラフ化を目的としたデータ送信に利用すること
- telemetry は継続的に蓄積される時系列データであり、軽量な通知や一時メッセージには利用しないこと
- 数値データだけでなく文字列や真偽値を含む JSON を送信できること
- telemetry の payload 構造は固定項目ではなく、device ごとに柔軟に定義できること

### 7.2 Device State

- desired state を取得できること
- reported state を送信できること
- 継続して保持すべき設定や目標状態は Device State で扱うこと
- Device State は server と device の間で状態や設定を同期するための機能であること
- `desired state` は server 側が device に対して望む状態を表すこと
- `reported state` は device が実際に保持している状態を表すこと
- 一時的な命令ではなく、継続して維持したい設定値や動作モードを扱うこと
- SDK 利用者が desired と reported の差分を理解しやすいように扱えること

### 7.3 Remote Command

- command を取得できること
- command の実行結果を送信できること
- 一回限りの実行命令は Remote Command で扱うこと
- Remote Command は restart や calibrate のような一回実行型の操作を扱うこと
- command は queue として server 側に登録され、device が取得して実行すること
- command には少なくとも `queued`、`fetched`、`succeeded`、`failed` の状態があること
- SDK は command 名と任意 payload を扱えること
- 継続設定を command で代用せず、継続設定は Device State で扱うこと

### 7.4 Messaging

- message を path 単位で送受信できること
- 軽量な任意メッセージ共有は Messaging で扱うこと
- 時系列データの保存やグラフ化を目的とするデータは Telemetry で扱うこと
- 継続設定は Device State で扱うこと
- 一回限りの命令は Remote Command で扱うこと
- device 向けの message 取得は path 完全一致を基本とすること
- Messaging は publish / subscribe 型に近い軽量メッセージ共有機能として扱うこと
- 複数の device またはブラウザが同じ path を共有してメッセージを送受信できること
- path と最終取得時刻を指定して新着 message を取得できること
- path を指定して直近 message を件数上限付きで取得できること
- message は取得時に削除するのではなく、保持期限の経過により削除されること
- device 間の軽い通知、ブラウザと device の簡易チャット、短期的な情報共有に利用できること
- 長期保持すべき設定値は Messaging ではなく Device State を利用すること
- ブラウザ管理画面では path をまたいだ一覧、検索、履歴参照を行えること

### 7.5 OTA

- OTA 情報を取得できること
- OTA は firmware、software、config file、certificate、related asset の配布を含むこと
- OTA はファームウェア更新だけでなく、device に配布する各種ファイルの取得にも利用できること
- 配布対象には firmware、software、config file、certificate、related asset を含められること
- OTA の対象は `active` 状態の device を前提とすること
- SDK は配布情報の取得と適用結果の報告を扱えること

### 7.6 Device Log

- device から任意ログを送信できること
- device の動作ログ、通信ログ、エラーログなどを server へ送信できること
- ログは event として記録され、少なくとも `debug`、`info`、`warn`、`error` のレベルを扱えること
- telemetry と異なり、ログは時系列分析よりも動作確認や障害調査を目的とすること

### 7.7 待機時間制御

- 取得系 API に対して待機時間を指定できること
- server から返される次回待機時間を利用できること

### 7.8 疎通確認

- SDK は設定された接続先に対する疎通確認を行えること
- 疎通確認は `https` 利用設定または完全な endpoint 文字列の scheme に従って実行されること
- 疎通確認のために HTTP を利用する場合も、明示的な設定を前提とすること
- 取得系 API は polling または短時間の long polling を前提とすること
- device は希望待機時間を指定できるが、実際の待機時間は server 側が決定できること
- server は応答時に `next_wait_sec` を返し、device は次回接続までの待機時間として利用できること
- 待機時間制御は server 負荷や通信頻度を調整するための仕組みとして利用すること

## 9. ドキュメント構成

- SDK 全体の概要は本文書で定義する
- 公開 HTTP API の詳細は `API.ja.md` で定義する
