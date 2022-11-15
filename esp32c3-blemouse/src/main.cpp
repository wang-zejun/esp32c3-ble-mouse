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


#define UP           0
#define DOWN         1
#define LEFT         2
#define RIGHT        3 

const int x = 3;
const int y = 2;
const int key = 4;

void(* resetFunc) (void) = 0;

boolean flag = false;
unsigned int reset = 0;
unsigned long startTime, temp , ttt;

void setup() {
  pinMode(board_CENTER, INPUT_PULLUP);
  pinMode(x, ANALOG);
  pinMode(y, ANALOG);
  // pinMode(board_LEFT, INPUT_PULLUP);
  // pinMode(board_RIGHT, INPUT_PULLUP);
  pinMode(board_led_left, OUTPUT);
  pinMode(board_led_right, OUTPUT);
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  bleMouse.begin();
  reset = 0;
  digitalWrite(board_led_left, HIGH);
}
void slide(uint8_t direction)
{
  switch (direction)
  {
  case UP:
      bleMouse.press(MOUSE_LEFT);           
      delay(20);                            
      startTime = millis();
      while(millis()<startTime+190) {        
        bleMouse.move(0,-10);                
        delay(19);                           
      }
      bleMouse.release(MOUSE_LEFT);         
      delay(20);
      startTime = millis();
      while(millis()<startTime+190) {
        bleMouse.move(0,10);
        delay(19);
      }
    break;
  case DOWN:
      bleMouse.press(MOUSE_LEFT);           
      delay(20);                             
      startTime = millis();
      while(millis()<startTime+190) {        
        bleMouse.move(0,10);                
        delay(19);                           
      }
      bleMouse.release(MOUSE_LEFT);         
      delay(20);
      startTime = millis();
      while(millis()<startTime+190) {
        bleMouse.move(0,-10);
        delay(19);
      }
    break;
  case LEFT:
      bleMouse.press(MOUSE_LEFT);           
      delay(20);                           
      startTime = millis();
      while(millis()<startTime+190) {        
        bleMouse.move(-10,0);                
        delay(19);                           
      }
      bleMouse.release(MOUSE_LEFT);         
      delay(20);
      startTime = millis();
      while(millis()<startTime+190) {
        bleMouse.move(10,0);
        delay(19);
      }
    break;
  case RIGHT:
      bleMouse.press(MOUSE_LEFT);           
      delay(20);                            
      startTime = millis();
      while(millis()<startTime+190) {        
        bleMouse.move(10,0);                
        delay(19);                           
      }
      bleMouse.release(MOUSE_LEFT);         
      delay(20);
      startTime = millis();
      while(millis()<startTime+190) {
        bleMouse.move(-10,0);
        delay(19);
      }
    break;
  default:
    break;
  }
}
void loop() {
  if(bleMouse.isConnected()) {
    reset = 1;
    digitalWrite(board_led_right, HIGH);

    if (analogRead(x) < 100 ) { 
      while (analogRead(x) < 100);
      slide(UP);
      delay(500);
    }

    if (analogRead(x) > 4000) { 
      while (analogRead(x) > 4000);
      slide(DOWN);
      delay(500);
    }

    if (analogRead(y) > 4000) { 
      while (analogRead(y) > 4000);
      slide(LEFT);
      delay(500);
    }

    if (analogRead(y) < 100) { 
      while (analogRead(y) < 100);
      slide(RIGHT);
      delay(500);
    }

    if (digitalRead(board_CENTER) != HIGH) { 
      while (digitalRead(board_CENTER) != HIGH);
      bleMouse.click(MOUSE_LEFT);                        
    }
  } else {
      digitalWrite(board_led_right, LOW);
      if(reset == 1) resetFunc();
  }
  delay(100);
}