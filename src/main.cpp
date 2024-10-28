#include <Arduino.h>
#include <WiFi.h>
//#include "I2S.h"
#include "Wav.h"
#include <SPIFFS.h>
#include <WebSocketsClient.h>
#include "AudioFileSourceICYStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"



const char *ssid = "XU";
const char *password = "888888881";
const char *serverAddress = "192.168.68.239";
int serverPort = 8088;

WebSocketsClient webSocket;

#define BUFFER_SIZE 200 * 16 // 设置缓冲区大小

int16_t communicationData[BUFFER_SIZE]; // 音频数据缓冲区
bool isSendingData = false;             // 控制是否发送数据的标志
const char *URL = "";

AudioGeneratorWAV *wav;          // WAV 解码器
AudioFileSourceHTTPStream *file; // 改为 HTTP 流
AudioFileSourceBuffer *buff;
AudioOutputI2S *out;

class Queue
{
public:
  void push(const char *value)
  {
    data.push_back(strdup(value));
  }

  const char *pop()
  {
    if (data.empty())
    {
      Serial.println("Queue is empty, no URL to play");
      return nullptr; // 返回 nullptr，避免抛出异常
    }
    const char *value = data.front();
    data.erase(data.begin());
    return value;
  }

  bool empty() const
  {
    return data.empty();
  }

  size_t size() const
  {
    return data.size();
  }

private:
  std::vector<const char *> data;
};
Queue urlQueue;

// Called when a metadata event occurs
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  (void)isUnicode; // Punt this ball for now
  char s1[32], s2[64];
  strncpy_P(s1, type, sizeof(s1));
  s1[sizeof(s1) - 1] = 0;
  strncpy_P(s2, string, sizeof(s2));
  s2[sizeof(s2) - 1] = 0;
  Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, s1, s2);
  Serial.flush();
}

// Called when there's a warning or error
void StatusCallback(void *cbData, int code, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1) - 1] = 0;
  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  Serial.flush();
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    isSendingData = false;
    Serial.println("[WSc] Disconnected!");
    break;
  case WStype_CONNECTED:
    Serial.println("[WSc] Connected to server");
    break;
  case WStype_TEXT:
    // 根据服务器的消息来控制是否发送数据
    if (strcmp((const char *)payload, "start") == 0)
    {
      isSendingData = true;
      Serial.println("Start sending data");
    }
    else if (strcmp((const char *)payload, "stop") == 0)
    {
      isSendingData = false;
      Serial.println("Stop sending data");
    }
    else if (strncmp((const char *)payload, "http", 4) == 0)
    {
      Serial.println("Stop sending data and play mp3");
      urlQueue.push((const char *)payload);
    }
    break;
  case WStype_BIN:
    Serial.printf("[WSc] Got binary data: %u\n", length);
    break;
  }
}

void connectwifi()
{
  if (WiFi.status() == WL_CONNECTED)
    return;
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  delay(200);
  WiFi.config(IPAddress(192, 168, 68, 108), // 设置静态IP位址
              IPAddress(192, 168, 68, 1),
              IPAddress(255, 255, 255, 0),
              IPAddress(192, 168, 68, 1));
  WiFi.mode(WIFI_STA);
  Serial.println("Connecting to WIFI");
  WiFi.begin(ssid, password);
  while ((!(WiFi.status() == WL_CONNECTED)))
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected");
  Serial.println("My Local IP is : ");
  Serial.println(WiFi.localIP());
}
// 线程任务函数，用于播放 WAV 音频
void audioTask(void *param)
{
  while (1)
  {
    if (!urlQueue.empty())
    {
      isSendingData = false;
      const char *url = urlQueue.pop();
      if (url == nullptr)
      {                                       // 检查是否为 nullptr
        vTaskDelay(10 / portTICK_PERIOD_MS); // 如果队列为空，继续等待
        continue;
      }
      
    }
    else
    {
      isSendingData = true;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); // 外层循环延迟
  }
}
void webSocketTask(void *param)
{
  size_t bytesRead;

  while (1)
  {
    connectwifi();
    // 必须定期调用以保持连接
    webSocket.loop();
    if (isSendingData)
    {
      //bytesRead = I2S_Read(communicationData, sizeof(communicationData));
      // 通过WebSocket发送音频数据
     // if (bytesRead > 0)
     // {
        webSocket.sendBIN((uint8_t *)communicationData, bytesRead);
     // }
    }
     vTaskDelay(10 / portTICK_PERIOD_MS);
  }
  
}
void setup()
{
  Serial.begin(115200);
  audioLogger = &Serial;

  // I2S_BITS_PER_SAMPLE_8BIT 配置的话，下句会报错，
  // 最小必须配置成I2S_BITS_PER_SAMPLE_16BIT
  // I2S_Init(I2S_MODE_RX, 16000, I2S_BITS_PER_SAMPLE_16BIT);

  Serial.println("I2S_Init....");
  //I2S_Init(I2S_MODE_RX, 16000, I2S_BITS_PER_SAMPLE_16BIT);
  Serial.println("connectwifi....");
  connectwifi();

      char *url = "http://192.168.68.239:8080/outwav/output1726548550.1189485.wav";
  
      Serial.printf("Playing %s\n", url);
      out = new AudioOutputI2S(0,0);
      out->SetPinout(10, 11, 9);
      //out->SetGain(1);
      // WAV 解码器和文件流初始化
      wav = new AudioGeneratorWAV();
      wav->RegisterStatusCB(StatusCallback, (void *)"wav");
      file = new AudioFileSourceHTTPStream(url);
      file->RegisterMetadataCB(MDCallback, (void *)"HTTP");
      buff = new AudioFileSourceBuffer(file, 2048);
      buff->RegisterStatusCB(StatusCallback, (void *)"buffer");
      wav->begin(buff, out);

      // 播放音频文件
      while (wav->isRunning())
      {
        if (!wav->loop())
        {
          break;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); // 保持播放流畅
      }

      // 播放结束后清理
      wav->stop();
      delete file;
      delete buff;
      delete wav;
      delete out;
      wav = nullptr;
      file = nullptr;
      buff = nullptr;
      out = nullptr;
      Serial.println("Waiting for URL");
      vTaskDelay(10 /portTICK_PERIOD_MS);

}

void loop()
{
  // 空循环，等待任务结束
  delay(1000);
}