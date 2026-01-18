#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <esp_now.h>

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

#define SDA_PIN 10
#define SCL_PIN 9

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= JOYSTICKS =================
#define THROTTLE_PIN 0   // лівий джойстик вертикаль
#define PITCH_PIN    1   // правий джойстик вертикаль
#define YAW_PIN      2   // правий джойстик горизонталь

#define DEADZONE 300

int thrCenter, pitchCenter, yawCenter;

// ================= ESP-NOW =================
// ❗ ВПИШИ MAC ДРОНА
uint8_t droneMAC[] = {0x24,0x6F,0x28,0xAA,0xBB,0xCC};

typedef struct {
  uint16_t throttle;
  int16_t pitch;
  int16_t yaw;
} ControlData;

ControlData data;

// ================= FUNCTIONS =================
int applyDeadzone(int v, int center) {
  if (abs(v - center) < DEADZONE) return center;
  return v;
}

void calibrate() {
  long t=0,p=0,y=0;
  for(int i=0;i<30;i++){
    t+=analogRead(THROTTLE_PIN);
    p+=analogRead(PITCH_PIN);
    y+=analogRead(YAW_PIN);
    delay(10);
  }
  thrCenter=t/30;
  pitchCenter=p/30;
  yawCenter=y/30;
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.setTextColor(SSD1306_WHITE);

  display.clearDisplay();
  display.setCursor(20,25);
  display.println("Calibrating...");
  display.display();
  calibrate();

  WiFi.mode(WIFI_STA);
  esp_now_init();

  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, droneMAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

// ================= LOOP =================
void loop() {
  int thr = applyDeadzone(analogRead(THROTTLE_PIN), thrCenter);
  int pit = applyDeadzone(analogRead(PITCH_PIN), pitchCenter);
  int yaw = applyDeadzone(analogRead(YAW_PIN), yawCenter);

  data.throttle = map(thr, 0, 4095, 0, 255);
  data.pitch    = map(pit, 0, 4095, -30, 30);
  data.yaw      = map(yaw, 0, 4095, -100, 100);

  esp_now_send(droneMAC, (uint8_t*)&data, sizeof(data));

  display.clearDisplay();
  display.setCursor(0,0);
  display.print("THR: "); display.println(data.throttle);
  display.print("PITCH: "); display.println(data.pitch);
  display.print("YAW: "); display.println(data.yaw);
  display.display();

  delay(30);
}
