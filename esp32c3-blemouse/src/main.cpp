#include <Arduino.h>

#include <BleMouse.h>

BleMouse bleMouse;

#define board_CENTER       4
#define board_UP           9
#define board_DOWN         5
#define board_LEFT         8
#define board_RIGHT        13 //13
#define board_led_left     12
#define board_led_right    13

const int x = 3;
const int y = 5;
const int key = 4;

void(* resetFunc) (void) = 0;

boolean flag = false;
unsigned int reset = 0;
unsigned long startTime, temp , ttt;

void setup() {
  pinMode(board_CENTER, INPUT_PULLUP);
  // pinMode(board_UP, INPUT_PULLUP);
  // pinMode(board_DOWN, INPUT_PULLUP);
  // pinMode(board_LEFT, INPUT_PULLUP);
  // pinMode(board_RIGHT, INPUT_PULLUP);
  pinMode(board_led_left, OUTPUT);
  pinMode(board_led_right, OUTPUT);
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  bleMouse.begin();
  reset = 0;
}
void loop() {

  if(bleMouse.isConnected()) {

    reset = 1;

    digitalWrite(board_led_left, HIGH);
    // if (digitalRead(board_UP) != HIGH) { 
    //   while (digitalRead(board_UP) != HIGH);
    if (analogRead(x) < 500 ) { 
      while (analogRead(x) < 500);
      Serial.println("Scroll up");
      startTime = millis();
      while(millis()<startTime+196) {
        bleMouse.move(0,0,1);
        delay(18);
      }
    }

    // if (digitalRead(board_DOWN) != HIGH) { 
    //   while (digitalRead(board_DOWN) != HIGH);
    if (analogRead(x) > 4000) { 
      while (analogRead(x) > 4000);
      Serial.println("Scroll down");
      startTime = millis();
      while(millis()<startTime+196) {
        bleMouse.move(0,0,-1);
        delay(18);
      }
    }

    // if (digitalRead(board_LEFT) != HIGH) { 
    //   while (digitalRead(board_LEFT) != HIGH);
    if (analogRead(y) > 4000) { 
      while (analogRead(y) > 4000);
      // Serial.println("Move mouse pointer left");
      // startTime = millis();
      // while(millis()<startTime+2000) {
      //   bleMouse.move(-1,0);
      //   delay(2);
      // }

      Serial.println("按住左键");
      bleMouse.press(MOUSE_LEFT);           
      delay(20);                           
      Serial.println("Move mouse pointer left");  
      startTime = millis();
      while(millis()<startTime+190) {        
        bleMouse.move(-10,0);                
        delay(19);                           
      }
      Serial.println("释放左键");
      bleMouse.release(MOUSE_LEFT);         
      delay(20);
      Serial.println("Move mouse pointer left");
      startTime = millis();
      while(millis()<startTime+190) {
        bleMouse.move(10,0);
        delay(19);
      }
    }

    // if (digitalRead(board_RIGHT) != HIGH) { 
    //   while (digitalRead(board_RIGHT) != HIGH);
    if (analogRead(y) < 500) { 
      while (analogRead(y) < 500);
      Serial.println("Move mouse pointer right");
      startTime = millis();
      while(millis()<startTime+2000) {
        bleMouse.move(1,0);
        delay(2);
      }
    }

    if (digitalRead(board_CENTER) != HIGH) { 
      while (digitalRead(board_CENTER) != HIGH);

      // Serial.println("按住左键");
      // bleMouse.press(MOUSE_LEFT);           
      // delay(20);                           
      // Serial.println("Move mouse pointer up");  
      // startTime = millis();
      // while(millis()<startTime+190) {        
      //   bleMouse.move(0,-10);                
      //   delay(19);                           
      // }
      // Serial.println("释放左键");
      // bleMouse.release(MOUSE_LEFT);         
      // delay(20);
      // Serial.println("Move mouse pointer down");
      // startTime = millis();
      // while(millis()<startTime+190) {
      //   bleMouse.move(0,10);
      //   delay(19);
      // }

      if(flag == false) flag = true;
      else flag = false;                       // 按下按钮为低电平
    }

    if(flag == true) {
      digitalWrite(board_led_right, HIGH);
      temp = random(30,100);      
      ttt =  temp * 10;                           //随机时间3~10S，可按照自己想法修改
      Serial.println("Move mouse pointer up");
      startTime = millis();
      while(millis()<startTime+180) {
        bleMouse.move(0,0,1);
        delay(18);
      }
      for(int i = 0; i < ttt; i++)
      {
        if (digitalRead(board_CENTER) != HIGH) { 
        while (digitalRead(board_CENTER) != HIGH);
        if(flag == false) flag = true;
        else flag = false;                       // 按下按钮为低电平
        break;
        }
        delay(10);
      }
    } else {
        digitalWrite(board_led_right, LOW);
        delay(10);
    }

  } else {
      digitalWrite(board_led_left, LOW);
      digitalWrite(board_led_right, LOW);
      if(reset == 1) resetFunc();
  }
  delay(10);
}