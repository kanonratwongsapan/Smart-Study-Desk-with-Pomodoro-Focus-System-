/*************************************************
   SMART POMODORO SYSTEM (ESP32)
   - LDR : ตรวจจับแสง (เปิด/ปิดผ่านเว็บได้)
   - Ultrasonic : ตรวจจับระยะ
   - Button & Web : เริ่ม / รีเซ็ต / ควบคุม LED & BUZZER
   - Buzzer : แจ้งเตือน (เปิด/ปิดผ่านเว็บได้)
   - State Machine : IDLE / FOCUS / BREAK (ทำครบ 2 รอบตัดจบอัตโนมัติ)
   - Telegram & Firebase
**************************************************/

#include <WiFi.h>
#include <WiFiManager.h>
#include <Firebase_ESP_Client.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// =====================
// API CONFIG
// =====================
#define API_KEY "AIzaSyAZQ015l7sFY1B8dleeZDopb5aPiYdKZZY"
#define DATABASE_URL "smartstudydesk-5f37f-default-rtdb.asia-southeast1.firebasedatabase.app"

#define BOT_TOKEN "8675066760:AAHOuzq90TuWWRwNAu-TdN4E4hT-s1w-r-k"
#define CHAT_ID "8053141395"

// =====================
// PIN CONFIG
// =====================
#define LDR 34
#define TRIG 5
#define ECHO 18

#define LED_GREEN 21
#define LED_YELLOW 22
#define LED_RED 23
#define LED_FOCUS 25

#define BUZZER 19
#define BUTTON 4

// =====================
// GLOBAL VARIABLES
// =====================
enum State { IDLE, FOCUS, BREAK };
volatile State currentState = IDLE;

volatile unsigned long previousMillis = 0;
volatile unsigned long interval = 0;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime = 0;
const unsigned long BOT_MTBS = 2000;

volatile float distance;
volatile int currentLightValue = 0;

// ✨ ตัวแปรควบคุมสวิตช์หลักจากหน้าเว็บ
volatile bool isLedEnabled = true;    // เปิด/ปิด LED
volatile bool isBuzzerEnabled = true; // เปิด/ปิด BUZZER

bool lastButtonState = HIGH;
bool alertState = false;
unsigned long alertMillis = 0;
const unsigned long alertInterval = 150;

bool isTooCloseReported = false;
unsigned long firebaseMillis = 0;
const unsigned long firebaseInterval = 2000;

volatile int pomodoroRound = 0;

// =====================
// THREAD FLAGS
// =====================
volatile bool flagMsgFocus = false;
volatile bool flagMsgBreak = false;
volatile bool flagMsgIdle = false;
volatile bool flagMsgClose = false;
volatile bool flagMsgAway = false;
volatile bool flagMsgDone = false;

TaskHandle_t NetworkTaskHandle;

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("START");
  
  WiFiManager wm;
  bool res = wm.autoConnect("Smart-Study-Desk");
  if(!res) {
    Serial.println("Failed to connect");
    ESP.restart();
  }
  Serial.println("Connected to WiFi!");

  disableCore0WDT();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.signer.test_mode = true;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  secured_client.setInsecure();
  bot.sendMessage(CHAT_ID, "🤖 ระบบ Smart Study Desk พร้อมใช้งานแล้ว!", "");

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_FOCUS, OUTPUT);

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, HIGH);
  pinMode(BUTTON, INPUT_PULLUP);

  xTaskCreatePinnedToCore(
    networkTask,      
    "NetworkTask",    
    24576,            
    NULL,            
    1,                
    &NetworkTaskHandle,
    0                
  );

  Serial.println("SYSTEM READY");
}

// =====================================================
// MAIN LOOP (CORE 1)
// =====================================================
void loop() {
  unsigned long currentMillis = millis();

  readButton(currentMillis);
  handleLDR();
  handleUltrasonic();
  handleStateMachine(currentMillis);

  delay(20);
}

// =====================================================
// NETWORK TASK (CORE 0)
// =====================================================
void networkTask(void * pvParameters) {
  for(;;) {
    unsigned long currentMillis = millis();

    // 1. ตรวจสอบและส่งแจ้งเตือน Telegram (อัปเดตข้อความให้ชัดเจนตามโหมด)
    if (flagMsgFocus) { bot.sendMessage(CHAT_ID, "🍅 **เข้าสู่โหมด FOCUS แล้ว!** (รอบที่ " + String(pomodoroRound) + ") ตั้งใจทำงานนะครับ", "Markdown"); flagMsgFocus = false; delay(50); }
    if (flagMsgBreak) { bot.sendMessage(CHAT_ID, "☕ **ได้เวลาพักแล้ว (BREAK)** (รอบที่ " + String(pomodoroRound) + ") ยืดเส้นยืดสายหน่อยครับ", "Markdown"); flagMsgBreak = false; delay(50); }
    if (flagMsgIdle) { bot.sendMessage(CHAT_ID, "🛑 **ตอนนี้ระบบอยู่ในโหมด IDLE (ว่าง)** กดปุ่มเพื่อเริ่มรอบใหม่ได้เลย", "Markdown"); flagMsgIdle = false; delay(50); }
    if (flagMsgClose) { bot.sendMessage(CHAT_ID, "⚠️ **เตือนภัย:** คุณก้มใกล้โต๊ะเกินไปแล้ว! ถนอมสายตาด้วยครับ", "Markdown"); flagMsgClose = false; delay(50); }
    if (flagMsgAway) { bot.sendMessage(CHAT_ID, "🚶‍♂️ ไม่พบผู้ใช้งานที่โต๊ะ ระบบกลับสู่โหมด IDLE อัตโนมัติ", ""); flagMsgAway = false; delay(50); }
    if (flagMsgDone) { bot.sendMessage(CHAT_ID, "🎉 **จบการทำงานครบ 2 รอบแล้ว!** เก่งมากครับ ระบบกลับสู่โหมด IDLE อัตโนมัติ ปิดไฟพักผ่อนได้เลย", "Markdown"); flagMsgDone = false; delay(50); }

    currentMillis = millis();
    
    // 2. รับคำสั่งแชทจาก Telegram
    if (currentMillis - bot_lasttime > BOT_MTBS) {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      while (numNewMessages) {
        for (int i = 0; i < numNewMessages; i++) {
          String chat_id = bot.messages[i].chat_id;
          String text = bot.messages[i].text;
          
          if (chat_id == CHAT_ID) {
            if (text == "/status") {
              String stateStr = (currentState == IDLE) ? "IDLE 🛑" : (currentState == FOCUS) ? "FOCUS 🍅" : "BREAK ☕";
              String msg = "📊 **สถานะปัจจุบัน**\nโหมด: " + stateStr + " (รอบที่ " + String(pomodoroRound) + "/2)\nระยะห่าง: " + String(distance) + " cm\nแสงสว่าง: " + String(currentLightValue);
              bot.sendMessage(chat_id, msg, "Markdown");
            }
            else if (text == "/focus") {
              pomodoroRound = 1;
              startFocus(millis());
            }
            else if (text == "/idle") { resetToIdle(); }
          }
          delay(10);
        }
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        delay(50);
      }
      bot_lasttime = millis();
    }

    currentMillis = millis();

    // 3. ส่งข้อมูลและรับคำสั่งจาก Firebase
    if (currentMillis - firebaseMillis >= firebaseInterval) {
      firebaseMillis = currentMillis;

      // อ่านคำสั่ง State (START / RESET)
      if (Firebase.RTDB.getString(&fbdo, "/commands/state")) {
        String webCmd = fbdo.stringData();
        if (webCmd == "START") {
          if (currentState == IDLE) {
            pomodoroRound = 1;
            startFocus(millis());
          }
          Firebase.RTDB.setString(&fbdo, "/commands/state", "NONE");
        }
        else if (webCmd == "RESET") {
          resetToIdle();
          Firebase.RTDB.setString(&fbdo, "/commands/state", "NONE");
        }
      }
      delay(10);

      // อ่านคำสั่ง LED (ON / OFF) จากเว็บ
      if (Firebase.RTDB.getString(&fbdo, "/commands/led")) {
        String ledCmd = fbdo.stringData();
        if (ledCmd == "ON") {
          isLedEnabled = true;
          Firebase.RTDB.setString(&fbdo, "/commands/led", "NONE");
        }
        else if (ledCmd == "OFF") {
          isLedEnabled = false;
          Firebase.RTDB.setString(&fbdo, "/commands/led", "NONE");
        }
      }
      delay(10);

      // ✨ อ่านคำสั่ง BUZZER (ON / OFF) จากเว็บ
      if (Firebase.RTDB.getString(&fbdo, "/commands/buzzer")) {
        String buzzerCmd = fbdo.stringData();
        if (buzzerCmd == "ON") {
          isBuzzerEnabled = true; // เปิดสวิตช์เสียง
          Firebase.RTDB.setString(&fbdo, "/commands/buzzer", "NONE");
        }
        else if (buzzerCmd == "OFF") {
          isBuzzerEnabled = false; // ปิดสวิตช์เสียง
          digitalWrite(BUZZER, HIGH); // บังคับให้เงียบทันที
          Firebase.RTDB.setString(&fbdo, "/commands/buzzer", "NONE");
        }
      }
      delay(10);
      
      Firebase.RTDB.setFloat(&fbdo, "/distance", distance);
      delay(10);
      Firebase.RTDB.setInt(&fbdo, "/light", currentLightValue);

      // อัปเดตสถานะไฟให้ตรงกับความเป็นจริง
      Firebase.RTDB.setString(&fbdo, "/leds/green", (isLedEnabled && currentLightValue >= 4095) ? "ON" : "OFF");
      Firebase.RTDB.setString(&fbdo, "/leds/yellow", (currentState == BREAK) ? "ON" : "OFF");
      Firebase.RTDB.setString(&fbdo, "/leds/red", (distance < 30) ? "ON" : "OFF");
      Firebase.RTDB.setString(&fbdo, "/leds/focus", (currentState == FOCUS) ? "ON" : "OFF");

      String modeStr = "IDLE";
      int timeLeft = 0;
      if (currentState == FOCUS) {
        modeStr = "FOCUS";
        timeLeft = (interval - (currentMillis - previousMillis)) / 1000;
      } else if (currentState == BREAK) {
        modeStr = "BREAK";
        timeLeft = (interval - (currentMillis - previousMillis)) / 1000;
      }
      
      Firebase.RTDB.setString(&fbdo, "/status/mode", modeStr);
      Firebase.RTDB.setInt(&fbdo, "/status/timeLeft", timeLeft);
      Firebase.RTDB.setInt(&fbdo, "/status/round", pomodoroRound);
    }

    delay(50);
  }
}

// =====================================================
// BUTTON CONTROL
// =====================================================
void readButton(unsigned long currentMillis) {
  bool currentButtonState = digitalRead(BUTTON);
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    if (currentState == IDLE) {
      pomodoroRound = 1;
      startFocus(currentMillis);
    }
    else {
      resetToIdle();
    }
  }
  lastButtonState = currentButtonState;
}

// =====================================================
// LDR CONTROL
// =====================================================
void handleLDR() {
  currentLightValue = analogRead(LDR);
  
  if (isLedEnabled && currentLightValue >= 4095) {
    digitalWrite(LED_GREEN, HIGH);
  } else {
    digitalWrite(LED_GREEN, LOW);  
  }
}

// =====================================================
// ULTRASONIC & BUZZER CONTROL
// =====================================================
void handleUltrasonic() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long dur = pulseIn(ECHO, HIGH, 30000);
  distance = (dur == 0) ? 999 : (dur * 0.034 / 2);

  if (distance < 30) {
    if (!isTooCloseReported && distance > 0) {
      flagMsgClose = true;
      isTooCloseReported = true;
    }

    digitalWrite(LED_RED, HIGH);
    
    // กระพริบเสียงเตือน เฉพาะเมื่อ isBuzzerEnabled เป็น true
    if (millis() - alertMillis >= alertInterval) {
      alertMillis = millis();
      alertState = !alertState;
      if (isBuzzerEnabled) {
        digitalWrite(BUZZER, alertState ? LOW : HIGH);
      } else {
        digitalWrite(BUZZER, HIGH); // ปิดเสียง
      }
    }
    
  } else {
    isTooCloseReported = false;
    digitalWrite(LED_RED, LOW);
    digitalWrite(BUZZER, HIGH);
  }

  if (distance > 80 && currentState != IDLE) {
    resetToIdle();
    flagMsgAway = true;
  }
}

// =====================================================
// STATE MACHINE
// =====================================================
void handleStateMachine(unsigned long currentMillis) {
  switch (currentState) {
    case IDLE:
      digitalWrite(LED_FOCUS, LOW);
      digitalWrite(LED_YELLOW, LOW);
      break;

    case FOCUS:
      digitalWrite(LED_FOCUS, HIGH);
      digitalWrite(LED_YELLOW, LOW);
      if (currentMillis - previousMillis >= interval) {
        startBreak(currentMillis);
      }
      break;

    case BREAK:
      digitalWrite(LED_FOCUS, LOW);
      digitalWrite(LED_YELLOW, HIGH);
      if (currentMillis - previousMillis >= interval) {
        if (pomodoroRound < 2) {
          pomodoroRound++;            
          startFocus(currentMillis);  
        } else {
          finishSession();            
        }
      }
      break;
  }
}

// =====================================================
// STATE FUNCTIONS
// =====================================================
void startFocus(unsigned long currentMillis) {
  currentState = FOCUS;
  interval = 25000;     // Demo 25 sec
  previousMillis = currentMillis;
  flagMsgFocus = true;
  beep(200);
}

void startBreak(unsigned long currentMillis) {
  currentState = BREAK;
  interval = 5000;      // Demo 5 sec
  previousMillis = currentMillis;
  flagMsgBreak = true;
  beep(500);
}

void resetToIdle() {
  currentState = IDLE;
  pomodoroRound = 0;
  digitalWrite(LED_FOCUS, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(BUZZER, HIGH);
  flagMsgIdle = true;
  beep(100);
}

void finishSession() {
  currentState = IDLE;
  pomodoroRound = 0;
  digitalWrite(LED_FOCUS, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(BUZZER, HIGH);
  flagMsgDone = true;
  
  beep(150); delay(100);
  beep(150); delay(100);
  beep(300);
}

// =====================================================
// BUZZER BEEP
// =====================================================
void beep(int durationMs) {
  // ✨ เช็คก่อนว่าสวิตช์ Buzzer ถูกเปิดอยู่หรือไม่
  if (!isBuzzerEnabled) return;

  digitalWrite(BUZZER, LOW);
  delay(durationMs);
  digitalWrite(BUZZER, HIGH);
}