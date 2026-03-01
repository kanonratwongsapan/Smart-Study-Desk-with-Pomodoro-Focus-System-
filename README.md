# Smart Study Desk with Pomodoro Focus System

## บทนำ (Overview)

Smart Study Desk with Pomodoro Focus System เป็นโครงงานระบบโต๊ะอ่านหนังสืออัจฉริยะที่พัฒนาด้วยเทคโนโลยี IoT โดยใช้บอร์ด ESP32 เป็นตัวควบคุมหลัก ร่วมกับเซนเซอร์ต่าง ๆ และระบบ Cloud เพื่อช่วยให้ผู้ใช้งานสามารถจัดการเวลาอ่านหนังสือตามหลัก Pomodoro Technique และสามารถตรวจสอบสถานะผ่าน Web Dashboard ได้แบบเรียลไทม์

ระบบสามารถตรวจจับการนั่งของผู้ใช้ ควบคุมไฟโต๊ะ แจ้งเตือนเมื่อครบเวลา และส่งข้อมูลไปยัง Firebase ผ่าน WiFi

---

## คุณสมบัติของระบบ (Features)

- ระบบจับเวลาแบบ Pomodoro (Focus / Break)
- ตรวจจับระยะด้วย Ultrasonic Sensor
- ตรวจจับแสงด้วย LDR Sensor
- แสดงสถานะด้วย LED
- แจ้งเตือนด้วย Buzzer
- เริ่มการทำงานด้วยปุ่มกด
- เชื่อมต่อ WiFi
- ส่งข้อมูลไปยัง Firebase
- แสดงผลผ่าน Web Dashboard
- ควบคุมระบบผ่านหน้าเว็บได้

---

## อุปกรณ์ที่ใช้ (Hardware)

- ESP32
- Ultrasonic Sensor
- LDR Sensor
- LED
- Buzzer
- Push Button
- Breadboard และสายไฟ
- แหล่งจ่ายไฟ

---

## ซอฟต์แวร์ที่ใช้ (Software)

- Arduino IDE
- Firebase Realtime Database
- WiFiManager Library
- Firebase ESP Client Library
- HTML / CSS / JavaScript สำหรับ Web Dashboard

---

## โครงสร้างการทำงานของระบบ (System Architecture)

Button → ESP32 → Sensors → WiFi → Firebase → Web Dashboard

ESP32 ทำหน้าที่เป็นตัวควบคุมหลัก รับค่าจากเซนเซอร์ และส่งข้อมูลไปยัง Cloud

---

## หลักการทำงานของระบบ (Working Principle)

1. ผู้ใช้กดปุ่มเพื่อเริ่มการทำงาน
2. ระบบเข้าสู่โหมด Focus และเริ่มจับเวลา
3. เมื่อครบเวลา ระบบส่งเสียงเตือนและเข้าสู่โหมด Break
4. ระบบจะสลับ Focus / Break อัตโนมัติ
5. Ultrasonic ตรวจจับว่ามีผู้ใช้งานหรือไม่
6. LDR ใช้ควบคุมไฟโต๊ะอ่านหนังสือ
7. ESP32 ส่งข้อมูลไปยัง Firebase ผ่าน WiFi
8. Web Dashboard แสดงสถานะแบบเรียลไทม์

---

## วิธีติดตั้ง (Installation)

1. ติดตั้ง Arduino IDE
2. ติดตั้ง ESP32 Board Package
3. ติดตั้ง Library ที่จำเป็น
   - WiFi.h
   - Firebase_ESP_Client.h
   - WiFiManager.h
4. เชื่อมต่อ ESP32 กับคอมพิวเตอร์
5. อัปโหลดโค้ดลงบอร์ด
6. เปิด Web Dashboard

---

## วิธีใช้งาน (How to use)

1. เปิดระบบ
2. กดปุ่มเริ่ม
3. ระบบเข้าสู่โหมด Focus
4. เมื่อครบเวลา จะมีเสียงแจ้งเตือน
5. สามารถดูสถานะผ่านหน้าเว็บได้

---

## แนวทางพัฒนาในอนาคต (Future Work)

- เพิ่มหน้าจอแสดงผล
- เพิ่มแอปมือถือ
- เพิ่มเซนเซอร์อื่น ๆ
- ปรับปรุง Web Dashboard

---

## Wokwi 

(https://wokwi.com/projects/457235900489975809)
