/*
 * Arduino 設定 :  
EVB : ESP32-CAM　 ESP32-S
Select :  ESP32 Wrover Module，此為ESP32通用版本。
partition Scheme 為 Huge AP(3M NO OTA)
Upload speed : 115200  


＠僅WebServer 不須安裝 LIB
  Arduino IDE V2.3.2 OK
  Arduino     V1.8.19S OK
  
  2024-06-15  CamerawebServer 基本功能測試
  參考文件 : https://twgo.io/ezcam  
  @可以辨識到人臉

  @增加 TFT Display 

*/
#include "esp_camera.h"
#include <WiFi.h>

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"


#include "FS.h"
#include "SD_MMC.h"  // SD Card ESP32
#include <EEPROM.h>  // read and write from flash memory
#define EEPROM_SIZE 1

#include "SPI.h"
#include <TFT_eSPI.h>       // Hardware-specific library
TFT_eSPI tft = TFT_eSPI();  // Invoke custom library

extern int8_t Face_detected;  //判斷是否有偵測到人臉
int8_t show_count = 0;

const char* ssid = "CCAP-1";
const char* password = "24831435";

void startCameraServer();

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  pinMode(3, INPUT_PULLUP);  //INPUT, INPUT_PULLUP,INPUT_PULLDOWN

  tft.init();  // Init ST7789 display 240x240 pixel
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);  //0:-0 1:landscape-90 2:-180 3:inv. landscape-270

  //顯示文字
  tft.setFreeFont(&FreeSerifBold12pt7b);  //設定字型，請參考Adafruit GFX函式庫的資料
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(10, 30);  //https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts
  tft.printf("CC WEBCamer v00");


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
  //init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t* s = esp_camera_sensor_get();
  //initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        //flip it back
    s->set_brightness(s, 1);   //up the blightness just a bit
    s->set_saturation(s, -2);  //lower the saturation
  }
  //drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Face_detected == true) {
    //TFT 顯示文字
    tft.setFreeFont(&FreeSerifBold12pt7b);  //設定字型，請參考Adafruit GFX函式庫的資料
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(10, 50);  //https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts
    tft.printf("Open Door ");
    Face_detected = false;
    show_count = 5;
  }

  delay(5000);

  if (show_count != 0)
      show_count--;

  if (show_count < 1) {
    //TFT 顯示文字
    // tft.setFreeFont(&FreeSerifBold12pt7b);  //設定字型，請參考Adafruit GFX函式庫的資料
    // tft.setTextColor(TFT_RED, TFT_BLACK);
    // tft.setTextSize(1);
    tft.setCursor(1, 1);  //https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts
    //tft.drawRect(0,60, 100, 10, TFT_RED); //X,Y W,H
    tft.fillScreen(TFT_RED);
  }
}
