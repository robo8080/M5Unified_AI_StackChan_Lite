# M5Unified_AI_StackChan_Lite
必要最小限の機能の軽量化AIｽﾀｯｸﾁｬﾝです。


* 音声合成にWeb版 VOICEVOX:ずんだもんを使用しています。
* 音声認識に"Google Cloud STT"か"OpenAI Whisper"のどちらかを選択できます。
<br>

### お願い ###
AIｽﾀｯｸﾁｬﾝがしゃべる動画を投稿するときは下記の様な表記を付けて下さい。<br>
ずんだもんの部分は使用した声の種類に変えて下さい(AIｽﾀｯｸﾁｬﾝのデフォルトはずんだもんです)。<br>

 #VOICEVOX:ずんだもん
<br>

### 対応しているデバイス ###
* M5StickC PLUS + Speaker 2 HAT
* AtomS3 + ATOMICスピーカーベース + PDMマイクユニット
* M5Stack Core2

### プログラムをビルドするのに必要な物 ###
* VSCode<br>
* PlatformIO<br>

使用しているライブラリ等は"platformio.ini"を参照してください。<br>

### 設定方法 ###

* 以下の内容を設定してください。

1. Wi-Fi設定。main.cppの23行目付近。<br>
"YOUR_WIFI_SSID"<br>
"YOUR_WIFI_PASS"<br>

2. APIキー設定。main.cppの32行目付近。<br>
"YOUR OPENAI APIKEY"<br>
"YOUR VOICEVOX APIKEY"<br>
"YOUR STT APIKEY"<br>

* 【注意】<br>"YOUR_STT_APIKEY"には"Google Cloud STTのAPIキー" または、"YOUR_OPENAI_APIKEY"と同じものを設定します。<br>
"YOUR_STT_APIKEY"に"YOUR_OPENAI_APIKEY"と同じものを設定した場合は音声認識にOpenAI Whisperが使われます。


### 使い方 ###

* ボタンAを押す(CoreS3の場合は額にタッチ)とマイクからの録音が始まり音声認識で会話できるようになります。<br>
録音時間は3秒程度です。<br>


---

### ChatGPTのAPIキー取得の参考リンク ###

* [ChatGPT API利用方法の簡単解説](https://qiita.com/mikito/items/b69f38c54b362c20e9e6/ "Title")<br>

### Web版 VOICEVOX のAPIキーの取得 ###

* Web版 VOICEVOX のAPIキーの取得方法は、このページ（[ttsQuestV3Voicevox](https://github.com/ts-klassen/ttsQuestV3Voicevox/ "Title")）の一番下の方を参照してください。)<br>

### Google Cloud Speech to TextのAPIキー取得の参考リンク ###

* [Speech-to-Text APIキーの取得／登録方法について](https://nicecamera.kidsplates.jp/help/feature/transcription/apikey/ "Title")<br>

