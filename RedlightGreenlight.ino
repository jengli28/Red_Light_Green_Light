// -----------------------------------------------------------------
// RedLightGreenLight
// Juwan English                                           29NOV2022
// This is a recreation of the "Squid Game" version of the childhood
// favorite game, Red Light Green Light.
//
// The game is single player and begins once the player presses the
// start button located on the breadboard. A series of red and green
// light flashes, followed by an alarm marks the game's commencement. 
//
// The player then has 60 seconds to move his piece from the bottom 
// of the board to the finish line. The player must tilt the board at 
// least 45 degrees in the direction of the finish line in order to 
// activiate the stepper motor roatation. A string is attached to the 
// motor, which will pull the player towards the finish line.
//
// **EDIT**
// The player must push his piece physically with his finger to move.
//
// The player can only move if he sees a green light. If the player is 
// ever "caught" moving during the red light phase, they are 
// permanently elimated!
//
// The player can estimate how much time is left by looking at the 7-
// segment LED, which will countdown from 6 in steps of 10 seconds.
// Should the player run out of time, they will be eliminated!
//
// GOOD LUCK!

// ----- LIBRARIES

#include <MPU6050.h>
#include <Wire.h>
#include <Stepper.h>
#include <AccelStepper.h>
#include <Servo.h>
#include <stdio.h>   // Including these headers
#include <stdlib.h>  // so that I can generate a random
#include <time.h>    // array of numbers
#include <ezBuzzer.h> // ezBuzzer library

// ----- CONSTs (won't change)
const int Echo = 13;
const int Trig = 12;
const int Button = 7;
const int Buzzer = 6;
const int GreenLED = 22;
const int RedLED = 23;
const int looking = 175;
const int not_looking = 0;
const int not_dead = 175;
const int dead = 0;
const int buttonInterval = 300; // number of millisecs between button readings
const int timerInterval = 10000; // number of milisecs between each countdown
const int a = 27;   // set digital pin 27 for segment a
const int b = 26;   // set digital pin 26 for segment b
const int c = 25;   // set digital pin 25 for segment c
const int d = 30;   // set digital pin 30 for segment d
const int e = 31;   // set digital pin 31 for segment e
const int f = 28;   // set digital pin 28 for segment f
const int g = 29;   // set digital pin 29 for segment g
const int dp = 24;  // set digital pin 24 for segment dp
const int readInterval = 50;

// notes in the melody:
int melody[] = {
  NOTE_E5, NOTE_E5, NOTE_E5,
  NOTE_E5, NOTE_E5, NOTE_E5,
  NOTE_E5, NOTE_G5, NOTE_C5, NOTE_D5,
  NOTE_E5,
  NOTE_F5, NOTE_F5, NOTE_F5, NOTE_F5,
  NOTE_F5, NOTE_E5, NOTE_E5, NOTE_E5, NOTE_E5,
  NOTE_E5, NOTE_D5, NOTE_D5, NOTE_E5,
  NOTE_D5, NOTE_G5
};

// note durations: 4 = quarter note, 8 = eighth note, etc, also called tempo:
int noteDurations[] = {
  8, 8, 4,
  8, 8, 4,
  8, 8, 8, 8,
  2,
  8, 8, 8, 8,
  8, 8, 8, 16, 16,
  8, 8, 8, 8,
  4, 4
};

// ----- VARIABLES (will change)
bool wasMoving = false;
bool greenStatus = true;
bool redStatus = false;
byte GreenLEDState = LOW;             // used to record whether the LEDs are on or off
byte RedLEDState = LOW;           //   LOW = off

long duration, distance, distance2, duration2;

int start;
int isComplete = 0;
int i = 0;
int timer = 6;
int lastStartButtonState = HIGH; // the previous state from the input pin

unsigned long currentMillis = 0;       // stores the value of millis() in each iteration of loop()
unsigned long currentMicros = 0;
unsigned long previousTimerMillis = 0; // stores the last time the TimerMillis was checked
unsigned long previousGreenLEDMillis = 0;
unsigned long previousRedLEDMillis = 0;
unsigned long previousMicros = 0;
unsigned long stepIntervalMicros = 0;

// ----- CLASSES
Servo LoneServo;
Servo KillerServo;
ezBuzzer buzzer(Buzzer); // create ezBuzzer object that attach to a pin;

//--Setup-----------------------------------------------------
void setup() {
  Serial.begin(9600);         // set baud rate
  LoneServo.attach(4);
  KillerServo.attach(5);
  int k;                      // set variable
  for (k = 24; k <= 31; k++) {
    pinMode(k, OUTPUT);       // set pin 24-31 as “output”
  }
  pinMode(Echo, INPUT);
  pinMode(Trig, OUTPUT);
  pinMode(Button, INPUT);
  pinMode(Buzzer, OUTPUT);
  //pinMode(LoneServo, OUTPUT);
  //pinMode(KillerServo, OUTPUT);
  pinMode(GreenLED, OUTPUT);
  digitalWrite(GreenLED, LOW);
  pinMode(RedLED, OUTPUT);
  digitalWrite(RedLED, LOW);
  
  Serial.println("GAME TIME");
  LoneServo.write(not_looking);       // LoneServo's start position
  KillerServo.write(not_dead);        // KillerServo's start position
  delay(1000);
}

// ----- FUNCTIONS
/////////////////////
//Seven Segment Codes
/////////////////////
void digital_0(void) {  // display number 0
  unsigned char j;
  digitalWrite(a, HIGH);
  digitalWrite(b, HIGH);
  digitalWrite(c, HIGH);
  digitalWrite(d, HIGH);
  digitalWrite(e, HIGH);
  digitalWrite(f, HIGH);
  digitalWrite(g, LOW);
  digitalWrite(dp, LOW);
}
void digital_1(void) {  // display number 1
  unsigned char j;
  digitalWrite(c, HIGH);        // set level as “high” for pin 25, turn on segment c
  digitalWrite(b, HIGH);        // turn on segment b
  for (j = 27; j <= 31; j++) {  // turn off other segments
    digitalWrite(j, LOW);
  }
  digitalWrite(dp, LOW);  // turn off segment dp
}
void digital_2(void) {  // display number 2
  unsigned char j;
  digitalWrite(b, HIGH);
  digitalWrite(a, HIGH);
  for (j = 29; j <= 31; j++)
    digitalWrite(j, HIGH);
  digitalWrite(dp, LOW);
  digitalWrite(c, LOW);
  digitalWrite(f, LOW);
}
void digital_3(void) {  // display number 3
  digitalWrite(g, HIGH);
  digitalWrite(a, HIGH);
  digitalWrite(b, HIGH);
  digitalWrite(c, HIGH);
  digitalWrite(d, HIGH);
  digitalWrite(dp, LOW);
  digitalWrite(f, LOW);
  digitalWrite(e, LOW);
}
void digital_4(void) {  // display number 4
  digitalWrite(c, HIGH);
  digitalWrite(b, HIGH);
  digitalWrite(f, HIGH);
  digitalWrite(g, HIGH);
  digitalWrite(dp, LOW);
  digitalWrite(a, LOW);
  digitalWrite(e, LOW);
  digitalWrite(d, LOW);
}
void digital_5(void) {  // display number 5
  unsigned char j;
  digitalWrite(a, HIGH);
  digitalWrite(b, LOW);
  digitalWrite(c, HIGH);
  digitalWrite(d, HIGH);
  digitalWrite(e, LOW);
  digitalWrite(f, HIGH);
  digitalWrite(g, HIGH);
  digitalWrite(dp, LOW);
}
void digital_6(void) {  // display number 6
  unsigned char j;
  for (j = 27; j <= 31; j++)
    digitalWrite(j, HIGH);
  digitalWrite(c, HIGH);
  digitalWrite(dp, LOW);
  digitalWrite(b, LOW);
}
//-------- THIS SECTION OF FUNCTIONS IS GOOD ---------//
void updatePlayerTime(){
  if (currentMillis - previousTimerMillis >= timerInterval){
    timer -= 1;
    if (timer == 6){
      digital_6();
    }
    if (timer == 5){
      digital_5();
    }
    if (timer == 4){
      digital_4();
    }
    if (timer == 3){
      digital_3();
    }
    if (timer == 2){
      digital_2();
    }
    if (timer == 1){
      digital_1();
    }
    if (timer == 0){
      digital_0();
      killPlayer();
    }
    //Serial.println(timer);
    previousTimerMillis += timerInterval;
    
  }
}
void soundStart(){
  for(int q=0;q<500;q++) {// output a frequency sound
    digitalWrite(Buzzer,HIGH);// sound
    delay(1);//delay1ms 
    digitalWrite(Buzzer,LOW);//not sound
    delay(1);//ms delay 
  }
}
void makeBlink(){
  for(int j = 0;j < 3;j++){
    digitalWrite(RedLED,HIGH);
    digitalWrite(GreenLED,LOW);
    delay(500); 
    digitalWrite(RedLED,LOW);
    digitalWrite(GreenLED,HIGH);
    delay(500);
  }
  digitalWrite(RedLED,LOW);
  digitalWrite(GreenLED,LOW);
  delay(500);
  digitalWrite(RedLED,HIGH);
}
float calc_xy_angles(float x, float y, float z){
   // Using x y and z from accelerometer, calculate x and y angles
   float x_val, y_val, z_val, result;
   unsigned short x2, y2, z2; //24 bit

   // Lets get the deviations from our baseline
   x_val = x-9.70;
   y_val = y-1.14;
   z_val = z+3.20;

   // Work out the squares
   x2 = (unsigned short)(x_val*x_val);
   y2 = (unsigned short)(y_val*y_val);
   z2 = (unsigned short)(z_val*z_val);

   //X Axis
   result=sqrt(y2+z2);
   result=x_val/result;
   float accel_angle_x = atan(result);

   return accel_angle_x;
}
void killPlayer(){
  KillerServo.write(dead);
  isComplete = HIGH;     
}
void switchLeds() {
  // this is the code that actually switches the LEDs on and off
  digitalWrite(GreenLED, GreenLEDState);
  digitalWrite(RedLED, RedLEDState);
}
void updateGreenLEDState(int randomInterval) {
  if (GreenLEDState == LOW) {
    if (currentMillis - previousGreenLEDMillis >= randomInterval) {
      delay(randomInterval);
      GreenLEDState = HIGH;
      RedLEDState = LOW;
      previousGreenLEDMillis += randomInterval;
    }
  }
  else {
    if (currentMillis - previousGreenLEDMillis >= randomInterval) {
      delay(randomInterval);
       GreenLEDState = LOW;
       RedLEDState = HIGH;
       previousGreenLEDMillis += randomInterval;
    } 
  }    
}
int lightIntervals[] = {
  1000,3000,4000,1000,3000,5000,2000,3000,4000,1000,2000,2000,2000,3000,5000,1000
};
void turnLone(){
  if(GreenLEDState == LOW){
    LoneServo.write(looking);
  }
  else{
    LoneServo.write(not_looking);
  }
}
// -------- THIS SECTION OF FUNCTIONS NEED WORK ------//
void peek_A_Boo(){
  if(RedLEDState == HIGH){
    for(int o =0;o<10;o++){
    digitalWrite(Trig, LOW); 
    delayMicroseconds(2); 
    digitalWrite(Trig, HIGH);
    delayMicroseconds(10); 
    digitalWrite(Trig, LOW);
    duration = pulseIn(Echo, HIGH);
    distance = duration/58.2;
    delay(500);
    digitalWrite(Trig, LOW); 
    delayMicroseconds(2); 
    digitalWrite(Trig, HIGH);
    delayMicroseconds(10); 
    digitalWrite(Trig, LOW);
    duration2 = pulseIn(Echo, HIGH);
    distance2 = duration2/58.2;
    Serial.print("Distance1=");
    Serial.println(distance);
    Serial.print("Distance2=");
    Serial.println(distance2);
    if(distance > 31 || distance2 > 31){
      distance = -1;
      distance2= -1;
    }
    if(distance - distance2 >1){
      killPlayer();
    }
    }
  }  
}

void deathMelody(int k){
  while(k < 5000){
    if (buzzer.getState() == BUZZER_IDLE){
        int length = sizeof(noteDurations) / sizeof(int);
        buzzer.playMelody(melody, noteDurations, length); // playing
      }
    if(k == 5000){
      if (buzzer.getState() != BUZZER_IDLE) {
        buzzer.stop() ; // stop
      }
    }
    k++;
  }
}
//--Main-----------------------------------------------------
void loop() {
  /* It's time for the first game! Its a simple game that you should be familiar with.
  *  But be warned, getting caught could cost you everything.
  ***** “You won’t get caught if you hide behind someone.” ~ Some player who probably died *****/

  buzzer.loop(); // MUST call the buzzer.loop() function in loop()
  srand(time(NULL));
  start = digitalRead(Button);
  isComplete = LOW;
  KillerServo.write(not_dead);
  LoneServo.write(not_looking);
  timer = 6;
  if(start == HIGH){                  // check if the button is pressed, if yes....
    makeBlink();                      // Visual Signal that the game has started
    soundStart();                     // Audible Signal that the game has started
    int k = 0;
    int j = 0;
    while(isComplete != HIGH){        // Gotta write this without delay functions. Hmmm..
      currentMillis = millis();
      currentMicros = micros();
      updatePlayerTime();
      updateGreenLEDState(lightIntervals[j]);
      switchLeds();
      turnLone();
      peek_A_Boo();
      deathMelody(k);
      j++;      
      if(j==16){
        j = 0;
      }
    }
  }
}

