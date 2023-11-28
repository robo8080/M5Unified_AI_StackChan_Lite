#include <Arduino.h>
#include <M5Unified.h>
#include <Avatar.h>

#include <AudioOutput.h>
#include "AudioFileSourceICYStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputM5Speaker.h"
#include "AudioFileSourceHTTPSStream.h"
#include "WebVoiceVoxTTS.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "rootCACertificate.h"
#include "rootCAgoogle.h"
#include <ArduinoJson.h>
#include "AudioWhisper.h"
#include "Whisper.h"
#include "Audio.h"
#include "CloudSpeechClient.h"
#include <deque>

#define WIFI_SSID   "YOUR_WIFI_SSID"
#define WIFI_PASS   "YOUR_WIFI_PASS"

// 保存する質問と回答の最大数
const int MAX_HISTORY = 5;

// 過去の質問と回答を保存するデータ構造
std::deque<String> chatHistory;

#define OPENAI_APIKEY "SET YOUR OPENAI APIKEY"
#define VOICEVOX_APIKEY "SET YOUR VOICEVOX APIKEY"
#define STT_APIKEY "SET YOUR STT APIKEY"

//---------------------------------------------
String OPENAI_API_KEY = "";
String VOICEVOX_API_KEY = "";
String STT_API_KEY = "";
String TTS_SPEAKER_NO = "3";
String TTS_SPEAKER = "&speaker=";
String TTS_PARMS = TTS_SPEAKER + TTS_SPEAKER_NO;

const char *URL="http://gitfile.oss-cn-beijing.aliyuncs.com/11-fanfare.mp3";

/// set M5Speaker virtual channel (0-7)
static constexpr uint8_t m5spk_virtual_channel = 0;
static AudioOutputM5Speaker out(&M5.Speaker, m5spk_virtual_channel);
AudioGeneratorMP3 *mp3;
AudioFileSourceICYStream *file;
AudioFileSourceBuffer *buff;

String speech_text = "";
String speech_text_buffer = "";
DynamicJsonDocument chat_doc(1024*10);
String json_ChatString = "{\"model\": \"gpt-3.5-turbo-1106\",\"messages\": [{\"role\": \"user\", \"content\": \"""\"}]}";
String Role_JSON = "";
String InitBuffer = "";

using namespace m5avatar;
Avatar avatar;

bool init_chat_doc(const char *data)
{
  DeserializationError error = deserializeJson(chat_doc, data);
  if (error) {
    Serial.println("DeserializationError");
    return false;
  }
  String json_str; //= JSON.stringify(chat_doc);
  serializeJsonPretty(chat_doc, json_str);  // 文字列をシリアルポートに出力する
//  Serial.println(json_str);
    return true;
}

String https_post_json(const char* url, const char* json_string, const char* root_ca) {
  String payload = "";
  WiFiClientSecure *client = new WiFiClientSecure;
  if(client) {
    client -> setCACert(root_ca);
    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
      HTTPClient https;
      https.setTimeout( 65000 ); 
  
      Serial.print("[HTTPS] begin...\n");
      if (https.begin(*client, url)) {  // HTTPS
        Serial.print("[HTTPS] POST...\n");
        // start connection and send HTTP header
        https.addHeader("Content-Type", "application/json");
        https.addHeader("Authorization", String("Bearer ") + OPENAI_API_KEY);
        int httpCode = https.POST((uint8_t *)json_string, strlen(json_string));
  
        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
  
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            payload = https.getString();
            Serial.println("//////////////");
            Serial.println(payload);
            Serial.println("//////////////");
          }
        } else {
          Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }  
        https.end();
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }
      // End extra scoping block
    }  
    delete client;
  } else {
    Serial.println("Unable to create client");
  }
  return payload;
}

//String json_string;
String chatGpt(String json_string) {
  String response = "";
Serial.print("chatGpt = ");
Serial.println(json_string);
  avatar.setExpression(Expression::Doubt);
  avatar.setSpeechText("考え中…");
  String ret = https_post_json("https://api.openai.com/v1/chat/completions", json_string.c_str(), root_ca_openai);
  avatar.setExpression(Expression::Neutral);
  avatar.setSpeechText("");
  Serial.println(ret);
  if(ret != ""){
    DynamicJsonDocument doc(2000);
    DeserializationError error = deserializeJson(doc, ret.c_str());
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      avatar.setExpression(Expression::Sad);
      avatar.setSpeechText("エラーです");
      response = "エラーです";
      delay(1000);
      avatar.setSpeechText("");
      avatar.setExpression(Expression::Neutral);
    }else{
      const char* data = doc["choices"][0]["message"]["content"];
      Serial.println(data);
      response = String(data);
      std::replace(response.begin(),response.end(),'\n',' ');
    }
  } else {
    avatar.setExpression(Expression::Sad);
    avatar.setSpeechText("わかりません");
    response = "わかりません";
    delay(1000);
    avatar.setSpeechText("");
    avatar.setExpression(Expression::Neutral);
  }
  return response;
}

void exec_chatGPT(String text) {
  static String response = "";
  Serial.println(InitBuffer);
  init_chat_doc(InitBuffer.c_str());
  // 質問をチャット履歴に追加
  chatHistory.push_back(text);
   // チャット履歴が最大数を超えた場合、古い質問と回答を削除
  if (chatHistory.size() > MAX_HISTORY * 2)
  {
    chatHistory.pop_front();
    chatHistory.pop_front();
  }

  for (int i = 0; i < chatHistory.size(); i++)
  {
    JsonArray messages = chat_doc["messages"];
    JsonObject systemMessage1 = messages.createNestedObject();
    if(i % 2 == 0) {
      systemMessage1["role"] = "user";
    } else {
      systemMessage1["role"] = "assistant";
    }
    systemMessage1["content"] = chatHistory[i];
  }

  String json_string;
  serializeJson(chat_doc, json_string);
  if(speech_text=="" && speech_text_buffer == "") {
    response = chatGpt(json_string);
    speech_text = response;
    // 返答をチャット履歴に追加
    chatHistory.push_back(response);
  } else {
    response = "busy";
  }
  // Serial.printf("chatHistory.max_size %d \n",chatHistory.max_size());
  // Serial.printf("chatHistory.size %d \n",chatHistory.size());
  // for (int i = 0; i < chatHistory.size(); i++)
  // {
  //   Serial.print(i);
  //   Serial.println("= "+chatHistory[i]);
  // }
  serializeJsonPretty(chat_doc, json_string);
  Serial.println("====================");
  Serial.println(json_string);
  Serial.println("====================");

}
void playMP3(AudioFileSourceBuffer *buff){
  mp3->begin(buff, &out);
}
String SpeechToText(bool isGoogle){
  Serial.println("\r\nRecord start!\r\n");

  String ret = "";
  if( isGoogle) {
    Audio* audio = new Audio();
    audio->Record();  
    Serial.println("Record end\r\n");
    Serial.println("音声認識開始");
    avatar.setSpeechText("わかりました");  
    CloudSpeechClient* cloudSpeechClient = new CloudSpeechClient(root_ca_google, STT_API_KEY.c_str());
    ret = cloudSpeechClient->Transcribe(audio);
    delete cloudSpeechClient;
    delete audio;
  } else {
    AudioWhisper* audio = new AudioWhisper();
    audio->Record();  
    delay(500);
    Serial.println("Record end\r\n");
    Serial.println("音声認識開始");
   avatar.setSpeechText("わかりました");  
    Whisper* cloudSpeechClient = new Whisper(root_ca_openai, OPENAI_API_KEY.c_str());
    ret = cloudSpeechClient->Transcribe(audio);
    delete cloudSpeechClient;
    delete audio;
  }
  return ret;
}


// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  (void) isUnicode; // Punt this ball for now
  // Note that the type and string may be in PROGMEM, so copy them to RAM for printf
  char s1[32], s2[64];
  strncpy_P(s1, type, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  strncpy_P(s2, string, sizeof(s2));
  s2[sizeof(s2)-1]=0;
  Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, s1, s2);
  Serial.flush();
}

// Called when there's a warning or error (like a buffer underflow or decode hiccup)
void StatusCallback(void *cbData, int code, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  // Note that the string may be in PROGMEM, so copy it to RAM for printf
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  Serial.flush();
}

void lipSync(void *args)
{
  float gazeX, gazeY;
  int level = 0;
  DriveContext *ctx = (DriveContext *)args;
  Avatar *avatar = ctx->getAvatar();
  for (;;)
  {
    level = abs(*out.getBuffer());
    if(level<100) level = 0;
    if(level > 15000)
    {
      level = 15000;
    }
    float open = (float)level/15000.0;
    avatar->setMouthOpenRatio(open);
    avatar->getGaze(&gazeY, &gazeX);
    avatar->setRotation(gazeX * 5);
    delay(50);
  }
}

void Avatar_setup() {
  switch (M5.getBoard())
  {
    case m5::board_t::board_M5StackCore2:
      break;
    case m5::board_t::board_M5StickCPlus:
      avatar.setScale(0.45);
      avatar.setPosition(5, 40);
      break;
    case m5::board_t::board_M5AtomS3:
      avatar.setScale(0.4);
      break;
    default:
      break;
  }
  avatar.setSpeechFont(&fonts::efontJA_16);
  avatar.init(); // start drawing
  avatar.addTask(lipSync, "lipSync");
}

/*
void hardware_model_check(){
  // run-time branch : hardware model check
  const char* name;
  switch (M5.getBoard())
  {
#if defined (CONFIG_IDF_TARGET_ESP32S3)
  case m5::board_t::board_M5StackCoreS3:
    name = "StackCoreS3";
    break;
  case m5::board_t::board_M5StampS3:
    name = "StampS3";
    break;
  case m5::board_t::board_M5AtomS3U:
    name = "ATOMS3U";
    break;
  case m5::board_t::board_M5AtomS3Lite:
    name = "ATOMS3Lite";
    break;
  case m5::board_t::board_M5AtomS3:
    name = "ATOMS3";
    break;
#elif defined (CONFIG_IDF_TARGET_ESP32C3)
  case m5::board_t::board_M5StampC3:
    name = "StampC3";
    break;
  case m5::board_t::board_M5StampC3U:
    name = "StampC3U";
    break;
#else
  case m5::board_t::board_M5Stack:
    name = "Stack";
    break;
  case m5::board_t::board_M5StackCore2:
    name = "StackCore2";
    break;
  case m5::board_t::board_M5StickC:
    name = "StickC";
    break;
  case m5::board_t::board_M5StickCPlus:
    name = "StickCPlus";
    break;
  case m5::board_t::board_M5StackCoreInk:
    name = "CoreInk";
    break;
  case m5::board_t::board_M5Paper:
    name = "Paper";
    break;
  case m5::board_t::board_M5Tough:
    name = "Tough";
    break;
  case m5::board_t::board_M5Station:
    name = "Station";
    break;
  case m5::board_t::board_M5Atom:
    name = "ATOM";
    break;
  case m5::board_t::board_M5AtomPsram:
    name = "ATOM PSRAM";
    break;
  case m5::board_t::board_M5AtomU:
    name = "ATOM U";
    break;
  case m5::board_t::board_M5TimerCam:
    name = "TimerCamera";
    break;
  case m5::board_t::board_M5StampPico:
    name = "StampPico";
    break;
#endif
  default:
    name = "Who am I ?";
    break;
  }
  // M5.Display.startWrite();
  // M5.Display.print("Core:");
  // M5.Display.println(name);
  Serial.println(">> hardware model check <<");
  Serial.print("Core:");
  Serial.println(name);
}
*/

void setup()
{ 
  auto cfg = M5.config();

  cfg.external_spk = false;    //
  cfg.internal_mic = true;

#if defined (ARDUINO_M5Stick_C)
  cfg.external_spk = true;    /// use external speaker (SPK HAT / ATOMIC SPK)
  cfg.external_speaker.hat_spk2       = true;
  cfg.internal_mic = true;
#endif

#if defined (ARDUINO_M5Stack_ATOMS3)
  cfg.external_spk = true;    /// use external speaker (SPK HAT / ATOMIC SPK)
  cfg.external_speaker.atomic_spk     = true;
  cfg.internal_mic = false;
#endif

  M5.begin(cfg);

  //hardware_model_check();

  { /// custom setting
    auto spk_cfg = M5.Speaker.config();
 
    spk_cfg.sample_rate = 96000; // default:64000 (64kHz)  e.g. 48000 , 50000 , 80000 , 96000 , 100000 , 128000 , 144000 , 192000 , 200000
    spk_cfg.task_pinned_core = APP_CPU_NUM;

    M5.Speaker.config(spk_cfg);
  }

#if defined (ARDUINO_M5Stack_ATOMS3)
    auto mic_cfg = M5.Mic.config();
    // M5AtomS3は外部マイク(PDMUnit)なので設定を行う。
    mic_cfg.pin_ws = 1;
    mic_cfg.pin_data_in = 2;
    M5.Mic.config(mic_cfg);
#endif
  M5.Speaker.begin();
  
#if defined (ARDUINO_M5Stick_C) || defined (ARDUINO_M5Stack_ATOMS3)
  M5.Display.setRotation(1);
#endif

  M5.Display.println("Connecting to WiFi");
  Serial.println("Connecting to WiFi");
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  M5.update();
  while (WiFi.status() != WL_CONNECTED) {
    M5.Display.println("...Connecting to WiFi");
    Serial.println("...Connecting to WiFi");
    delay(1000);   
  }
  Serial.println("Connected");
  M5.Display.println("Connected");

  delay(1000);
  M5.Display.clearDisplay();

  M5.Speaker.setVolume(210);
  /// The setAllChannelVolume function can be set the all virtual channel volume in the range of 0-255. (default : 255)
  M5.Speaker.setAllChannelVolume(255);
  OPENAI_API_KEY = OPENAI_APIKEY;
  VOICEVOX_API_KEY = VOICEVOX_APIKEY;
  STT_API_KEY = STT_APIKEY;

  //audioLogger = &Serial;
  mp3 = new AudioGeneratorMP3();
  //mp3->RegisterStatusCB(StatusCallback, (void*)"mp3");
  init_chat_doc(json_ChatString.c_str());
  serializeJson(chat_doc, InitBuffer);

  Avatar_setup();
}

void loop()
{
  static int lastms = 0;

  M5.update();
  if (M5.BtnA.wasPressed())
  {
    M5.Speaker.tone(1000, 100);
        delay(1000);
        avatar.setExpression(Expression::Happy);
        avatar.setSpeechText("御用でしょうか？");
        M5.Speaker.end();
        String ret;
        if(OPENAI_API_KEY != STT_API_KEY){
          Serial.println("Google STT");
          ret = SpeechToText(true);
        } else {
          Serial.println("Whisper STT");
          ret = SpeechToText(false);
        }
        Serial.println("音声認識終了");
        Serial.println("音声認識結果");
        if(ret != "") {
          Serial.println(ret);
            exec_chatGPT(ret);
        } else {
          Serial.println("音声認識失敗");
          avatar.setExpression(Expression::Sad);
          avatar.setSpeechText("聞き取れませんでした");
          delay(2000);
          avatar.setSpeechText("");
          avatar.setExpression(Expression::Neutral);
        } 
        M5.Speaker.begin();
  }

  if(speech_text != ""){
    avatar.setExpression(Expression::Happy);
    speech_text_buffer = speech_text;
    speech_text = "";
    Voicevox_tts((char*)speech_text_buffer.c_str(), (char*)TTS_PARMS.c_str());
  }

  if (mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
      if(file != nullptr){delete file; file = nullptr;}
      Serial.println("mp3 stop");
      avatar.setExpression(Expression::Neutral);
      speech_text_buffer = "";
    }
    delay(1);
  }
}
