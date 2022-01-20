
#include <Arduino.h>
#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"

#include "credentials.h"



#define PIN_INDICATOR_LED 33
#define INDICATOR_ON LOW
#define INDICATOR_OFF HIGH
#define PIN_FLASH 4
#define FLASH_ON HIGH
#define FLASH_OFF LOW

// #define PIN_HOLD_VCC 12
// #define HOLD_VCC_ON HIGH
// #define ALLOW_VCC_OFF LOW

#define PIN_PIR_SENSOR GPIO_NUM_12
#define PIR_ACTIVE_STATE 1




String serverName = "192.168.1.125";   // REPLACE WITH YOUR Raspberry Pi IP ADDRESS
//String serverName = "example.com";   // OR REPLACE WITH YOUR DOMAIN NAME

String serverPath = "/upload_amcam";     // The default serverPath should be upload.php

const int serverPort = 80;

WiFiClient client;

// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

const int timerInterval = 120000;    // time between each HTTP POST image
unsigned long previousMillis = 0;   // last time image was sent

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}


String sendPhoto()
{
  digitalWrite(PIN_FLASH,FLASH_ON);
  String getAll;
  String getBody;

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }
  
  Serial.println("Connecting to server: " + serverName);

  if (client.connect(serverName.c_str(), serverPort)) {
    Serial.println("Connection successful!");    
    String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"imageFile\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--RandomNerdTutorials--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;
  
    client.println("POST " + serverPath + " HTTP/1.1");
    client.println("Host: " + serverName);
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=RandomNerdTutorials");
    client.println();
    client.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0; n<fbLen; n=n+1024) {
      if (n+1024 < fbLen) {
        client.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        client.write(fbBuf, remainder);
      }
    }   
    client.print(tail);
    
    esp_camera_fb_return(fb);
    
    int timoutTimer = 10000;
    long startTimer = millis();
    boolean state = false;
    
    while ((startTimer + timoutTimer) > millis()) {
      Serial.print(".");
      delay(100);      
      while (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (getAll.length()==0) { state=true; }
          getAll = "";
        }
        else if (c != '\r') { getAll += String(c); }
        if (state==true) { getBody += String(c); }
        startTimer = millis();
      }
      if (getBody.length()>0) { break; }
    }
    Serial.println();
    client.stop();
    Serial.println(getBody);
  }
  else {
    getBody = "Connection to " + serverName +  " failed.";
    Serial.println(getBody);
  }
  digitalWrite(PIN_FLASH,FLASH_OFF);
  return getBody;
}

void start_wifi()
{

  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("waiting for wifi");
    delay(500);
  }
  Serial.println();
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  // No longer used: Very first job, hold Vcc
  // pinMode(PIN_HOLD_VCC,OUTPUT);
  // digitalWrite(PIN_HOLD_VCC,HOLD_VCC_ON);
  
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  
  Serial.begin(115200);
  delay(2000);

  Serial.println("Started....");

  print_wakeup_reason();

  // for (uint8_t i=10;i>0;i--)
  // {
  //   Serial.printf("%d\n",i);
  //   delay(1000);
  // }

  start_wifi();

  pinMode(PIN_INDICATOR_LED,OUTPUT);
  digitalWrite(PIN_INDICATOR_LED,INDICATOR_ON);
  pinMode(PIN_FLASH,OUTPUT);
  digitalWrite(PIN_FLASH,FLASH_OFF);

  

  


  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // init with high specs to pre-allocate larger buffers
  if(psramFound()){
    
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 10;  //0-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;  //0-63 lower number means higher quality
    config.fb_count = 1;
  }
  Serial.printf("Using PSRAM?- %s\n",psramFound()?"YES":"NO");
  // camera init
  esp_err_t err = esp_camera_init(&config);
  Serial.println("Camera init'd");
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(2000);
    ESP.restart();
  }
  //start_wifi();
  Serial.println("About to send photo...");
  sendPhoto(); 
  digitalWrite(PIN_INDICATOR_LED,INDICATOR_OFF);

  // Cut power!- no longer used!
  // Serial.println("About to drop power");
  // delay(500);
  // digitalWrite(PIN_HOLD_VCC,ALLOW_VCC_OFF);



  // Set wakeup source as gpio12
  esp_sleep_enable_ext0_wakeup(PIN_PIR_SENSOR,PIR_ACTIVE_STATE);
  Serial.println("going to sleep now...");
  delay(1000);
  esp_deep_sleep_start();
  Serial.println("This should never be seen!");
}

void loop()
{
  Serial.println("This is never called!!");
}
