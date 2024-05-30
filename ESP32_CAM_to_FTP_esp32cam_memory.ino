#include "esp_camera.h"
#include <WiFi.h>
#include <FTPClient.h>
#include "SPIFFS.h" // 包含 SPIFFS 文件系统的头文件
#include <ctime> // 包含 time 函数的头文件
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "FS.h"

//定義WiFi網路訊息
const char* ssid = "網路名稱";  //網路名 須為2.4G
const char* password = "網路密碼"; //網路密碼


//FTP伺服器訊息
const char* ftpServer = "FTP伺服器IP";
const char* ftpUser = "FTP使用者";
const char* ftpPassword = "FTP密碼";

//======================================== CAMERA_MODEL_AI_THINKER GPIO.
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
//======================================== 

// LED Flash PIN (GPIO 4)
#define FLASH_LED_PIN 4
//String getDateTime(); 
// 創建 FTP 客户端對象
FTPClient ftp(SPIFFS);

int i;
int count=0;
void setup() {
  // put your setup code here, to run once:
  // Disable brownout detector.
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  Serial.begin(115200);
  Serial.println();
  delay(1000);
  // 初始化 SPIFFS 文件系统
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to initialize SPIFFS");
    return;
  }

  checkFileSystemSpace(); // 檢查檔案系統空間

  pinMode(FLASH_LED_PIN, OUTPUT);
  
  // Setting the ESP32 WiFi to station mode.
  Serial.println();
  Serial.println("Setting the ESP32 WiFi to station mode.");
  WiFi.mode(WIFI_STA);

  //---------------------------------------- The process of connecting ESP32 CAM with WiFi Hotspot / WiFi Router.
  Serial.println();
  Serial.print("Connecting to : ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  // The process timeout of connecting ESP32 CAM with WiFi Hotspot / WiFi Router is 20 seconds.
  // If within 20 seconds the ESP32 CAM has not been successfully connected to WiFi, the ESP32 CAM will restart.
  // I made this condition because on my ESP32-CAM, there are times when it seems like it can't connect to WiFi, so it needs to be restarted to be able to connect to WiFi.
  int connecting_process_timed_out = 20; //--> 20 = 20 seconds.
  connecting_process_timed_out = connecting_process_timed_out * 2;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    //digitalWrite(FLASH_LED_PIN, HIGH);
    delay(250);
    digitalWrite(FLASH_LED_PIN, LOW);
    delay(250);
    if(connecting_process_timed_out > 0) connecting_process_timed_out--;
    if(connecting_process_timed_out == 0) {
      Serial.println();
      Serial.print("Failed to connect to ");
      Serial.println(ssid);
      Serial.println("Restarting the ESP32 CAM.");
      delay(1000);
      ESP.restart();
    }
  }

  digitalWrite(FLASH_LED_PIN, LOW);
  
  Serial.println();
  Serial.print("Successfully connected to ");
  Serial.println(ssid);
  //Serial.print("ESP32-CAM IP Address: ");
  //Serial.println(WiFi.localIP());
  //---------------------------------------- 

  //---------------------------------------- Set the camera ESP32 CAM.
  Serial.println();
  Serial.println("Set the camera ESP32 CAM...");
  
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
    config.frame_size = FRAMESIZE_SXGA;
    config.jpeg_quality = 10;  //0-63 lower number means higher quality
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_SXGA;
    config.jpeg_quality = 10;  //0-63 lower number means higher quality
    config.fb_count = 1;
  }
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    Serial.println();
    Serial.println("Restarting the ESP32 CAM.");
    delay(1000);
    ESP.restart();
  }

  sensor_t * s = esp_camera_sensor_get();

  // Selectable camera resolution details :
  // -UXGA   = 1600 x 1200 pixels
  // -SXGA   = 1280 x 1024 pixels
  // -XGA    = 1024 x 768  pixels
  // -SVGA   = 800 x 600   pixels
  // -VGA    = 640 x 480   pixels
  // -CIF    = 352 x 288   pixels
  // -QVGA   = 320 x 240   pixels
  // -HQVGA  = 240 x 160   pixels
  // -QQVGA  = 160 x 120   pixels
  s->set_framesize(s, FRAMESIZE_SXGA);  //--> UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA

  Serial.println("Setting the camera successfully.");
  Serial.println();

  delay(1000);

  Serial.println();
  Serial.println("ESP32-CAM captures and sends photos to the server every 60 seconds.");
  Serial.println();
  delay(2000);

}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis % 60000 == 0) {
    i=0;
    captureAndUploadImage();
  }
}

void captureAndUploadImage(){
  Serial.println();
  Serial.println("Taking a photo...");

  camera_fb_t * fb = NULL;
  bool captured = false;

  for(int i = 0; i <= 3; i++){
    fb = esp_camera_fb_get();
    if(fb && fb->len > 0) {
      Serial.print("Image size: ");
      Serial.println(fb->len);
      captured = true;
      break;
    }
  }
  if(!captured) {
    Serial.println("Failed to capture image");
    return;
  }
  
  Serial.println("Taking a photo was successful.");

  // 初始化 FTP 客户端
  FTPClient::ServerInfo serverInfo(ftpUser, ftpPassword, ftpServer);
  ftp.begin(serverInfo);

  // 将图像數據寫入本地文件
  File file = SPIFFS.open("/image.jpg", FILE_WRITE);
  if (!file) {
    Serial.println("Failed to create local file");
    esp_camera_fb_return(fb);
    return;
  }
  size_t bytesWritten = file.write(fb->buf, fb->len);
  file.close();
  esp_camera_fb_return(fb);

  Serial.print("Bytes written to local file: ");
  Serial.println(bytesWritten);

  // 检查文件大小
  File localFile = SPIFFS.open("/image.jpg", FILE_READ);
  if (!localFile) {
    Serial.println("Failed to open local file for reading");
    return;
  }
  Serial.print("Local file size: ");
  Serial.println(localFile.size());
  localFile.close();


  if(i<1){
    i++;
    Serial.println("Run again.");
    localFile.close();
    SPIFFS.remove("/image.jpg");
    delay(100); // 等待一段时间确保文件已写入
    esp_camera_fb_return(fb);
    Serial.print("Free heap after releasing framebuffer: ");
    Serial.println(ESP.getFreeHeap());
    delay(200);
    captureAndUploadImage();
    return;
  }

  // 检查文件是否为空
  if (bytesWritten == 0) {
    Serial.println("Local file size is 0, trying again...");
    captureAndUploadImage(); // Retry capturing and uploading
    return;
  }
  
  // 获取当前时间作为文件名
  String fileName = "/home/meijiali/Pictures/" + String(count) + ".jpg";
  count++;

  // 传输文件
  FTPClient::TransferType direction = FTPClient::FTP_PUT; // 上传文件
  FTPClient::Status status = ftp.transfer("/image.jpg", fileName, direction);
  
  // 检查传输状态
  if (status.result == FTPClient::OK) {
    Serial.println("File transfer successful");
  } else {
    Serial.println("File transfer failed");
    Serial.println("Error code: " + String(status.code));
    Serial.println("Error description: " + status.desc);
  }
}
String getDateTime() {
    // 获取当前时间
    time_t now = time(nullptr);
    struct tm *timeinfo;
    timeinfo = localtime(&now);

    // 格式化时间字符串
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", timeinfo);

    return String(buffer);
}

void checkFileSystemSpace() {
  size_t totalBytes = SPIFFS.totalBytes();
  size_t usedBytes = SPIFFS.usedBytes();

  Serial.print("Total space: ");
  Serial.println(totalBytes);
  Serial.print("Used space: ");
  Serial.println(usedBytes);
  Serial.print("Free space: ");
  Serial.println(totalBytes - usedBytes);

  if (totalBytes - usedBytes < 100000) {
    Serial.println("Insufficient file system space");
    // Handle insufficient space here, such as deleting old files or increasing file system size.
  }
}
