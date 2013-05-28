/*

Arduino Laser Tag

Copyright (c) 2013, Kevin Anthony (kevin@anthonynet.org
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met: 

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer. 
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
 
#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
#include "IRremote.h"

// Defining bitmasks for custom letters on the segment display
#define BM_A     0b01110111
#define BM_D     0b00111111
#define BM_E     0b01111001
#define BM_G     0b00111101
#define BM_O     0b00111111
#define BM_P     0b01110011
#define BM_S     0b01101101
#define BM_U     0b00111110
#define BM_DASH  0b01000000
#define BM_CLEAR 0b00000000


Adafruit_7segment matrix = Adafruit_7segment();

const int irSensorPin  = 2;    // IR Sensor
const int fireLedPin   = 3;    // IR LED
const int triggerPin   = 4;    // Trigger Switch
const int reloadPin    = 6;    // Reloading Switch
const int myAmmoLedPin = 9;    // Out of Ammo LED
const int speakerPin   = 12;   // Piezo Speaker
const int hitLedPin    = 13;   // Hit Indicator

// Game play variables 
int myCode             = 1;    // Unused at the moment
int maxHealth          = 3;    // Number of times you can get shot before dying
int myHealth           = maxHealth;
int maxAmmo            = 30;   // Amount of ammo returned on a reload
int ammoCount          = 30;   // Starting ammo
int deadTime           = 3000; // Number of ms you stay dead
int deathCount         = 0;

// Initial states for state stores
int triggerState       = LOW;       
int hitState           = HIGH;
int reloadState        = LOW;
int lastTriggerState   = LOW;   
int lastHitState       = HIGH;
int lastReloadState    = LOW;

// Setup the IR sender and receiver routines.
IRsend irsend;
IRrecv irrecv(irSensorPin);
decode_results results;      // Stores the IR receiver results

// Setup routine, runs once at beginning.
void setup() {
  // initialize the button/sensor pins as input:
  pinMode(triggerPin, INPUT);
  pinMode(irSensorPin, INPUT);
  pinMode(reloadPin, INPUT);
  // initialize the LED/Speaker as output:
  pinMode(fireLedPin, OUTPUT);
  pinMode(myAmmoLedPin, OUTPUT);
  pinMode(hitLedPin, OUTPUT);
  pinMode(speakerPin, OUTPUT);
  // initialize serial communication for debug
  Serial.begin(9600);
    
  // Adafruit I2C 7-Segment Display backpack initialization  
  matrix.begin(0x70);  // pass in the address 
  matrix.setBrightness(4);
  
  // Write out the initial display
  matrix.writeDigitRaw(1, BM_G);
  matrix.writeDigitRaw(3, BM_O);
  matrix.writeDisplay();  
  
  // Start the receiver listening
  irrecv.enableIRIn(); 
  
  // Startup tone generator
  for (int i = 6;i > 0;i--) {
    digitalWrite(hitLedPin, HIGH);
    playTone(900*i, 50);
    digitalWrite(hitLedPin, LOW);
    delay(50);
  }

  Serial.println("Ready.");  
}

// Main loop
void loop() {  
  
  // Check if there is a message waiting that can be decoded
  if (irrecv.decode(&results)) {
    Serial.println(results.value, DEC);

    // Check if it is a valid hit routine
    // This should be the firing IR code of who you are NOT
    // Player1 = 3978641416;
    // Player2 = 275977667;
    // TODO: Need to change this to accept a range so more than two 'teams' can be in play
    //       Probalby shouldn't be doing this gross decimal parsing too on the binary values.
    
    if (results.value == 275977667) {
      Serial.println("HIT HIT HIT");
      digitalWrite(hitLedPin, HIGH);
      // TODO: Less lame hit sound
      playTone(2500, 200);
      
      // Reduce their health and then check if they should be dead or not
      myHealth--;
      if (myHealth <= 0) {
        Serial.println("DEAD!"); 
        deathCount++;
        matrix.blinkRate(1);
        matrix.writeDigitRaw(0, BM_D);
        matrix.writeDigitRaw(1, BM_E);
        matrix.writeDigitRaw(3, BM_A);
        matrix.writeDigitRaw(4, BM_D);
        matrix.writeDisplay();
        playTone(5000, 1000);      
        delay(deadTime);
        matrix.blinkRate(0);
        matrix.print(deathCount, DEC);
        matrix.writeDigitRaw(0, BM_S);
        matrix.writeDigitRaw(1, BM_DASH);
        matrix.writeDisplay();        
        delay(2000);
        matrix.writeDigitRaw(0, BM_CLEAR);
        matrix.writeDigitRaw(1, BM_G);
        matrix.writeDigitRaw(3, BM_O);
        matrix.writeDigitRaw(4, BM_CLEAR);
        matrix.writeDisplay();  
        myHealth = maxHealth;
        ammoCount = maxAmmo;       
        Serial.println("Ready again!"); 

      } else {
        Serial.print("Health: ");
        Serial.println(myHealth); 
      }
      digitalWrite(hitLedPin, LOW);

      
    } else {
      Serial.println("IR signal received but not recognized");
      digitalWrite(hitLedPin, LOW);

    }

    irrecv.resume(); // Receive the next value
  }  

  // read the pushbutton input pins
  triggerState = digitalRead(triggerPin);
  reloadState = digitalRead(reloadPin);

  // Check if the state has changed for the reload pin and then perform a reload if it's been pushed
  if (reloadState != lastReloadState) {
    if (reloadState == HIGH) {
       Serial.println("Reloading!"); 
       ammoCount = maxAmmo;
       matrix.blinkRate(0);
       matrix.print(ammoCount, DEC);
       matrix.writeDisplay();
       playTone(1000, 100);
       playTone(500, 100); 

    }
  }   
  
  // compare the triggerState to its previous state
  if (triggerState != lastTriggerState) {
    if (ammoCount > 0) {    
      if (triggerState == HIGH) {
        ammoCount--;
        Serial.println("Trigger on");
        Serial.print("Ammo:  ");
        Serial.println(ammoCount);

        // TODO: Less lame firing sound
        playTone(600, 50);
        playTone(500, 50);
        playTone(400, 50);
        playTone(300, 50);
      
        // Fire the IR LED (3x to try and make sure it is received and to keep to Sony protocol)
        // This should be the firing IR code for who you are
        // Player1 = 0x00B
        // Player2 = 0xa90 
        
        irsend.sendSony(0x00B, fireLedPin);
        irsend.sendSony(0x00B, fireLedPin);
        irsend.sendSony(0x00B, fireLedPin);

        irrecv.enableIRIn();
        
      } else {
        Serial.println("Trigger off"); 
      }

      matrix.print(ammoCount, DEC);
      matrix.writeDisplay();      
  
    } else {
      // Out of ammo so flash the display accordingly and play a error noise if they try and fire  
      matrix.writeDigitNum(0, 0);
      matrix.writeDigitNum(1, 0);
      matrix.writeDigitNum(3, 0);
      matrix.writeDigitNum(4, 0);
      matrix.blinkRate(2);
      matrix.writeDisplay();
      //digitalWrite(fireLedPin, LOW);
      digitalWrite(myAmmoLedPin, HIGH); 
      Serial.println("Out of myAmmo!");
      playTone(5000, 100);
      //delay(40);

    }
      
    lastTriggerState = triggerState;
   }
}

// Play a sound on the piezo
void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(speakerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(speakerPin, LOW);
    delayMicroseconds(tone);
  }
}
