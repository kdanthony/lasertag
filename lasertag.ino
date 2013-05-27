/*

Arduino Laser Tag
2013 Kevin Anthony
kevin@anthonynet.org

*/
 
#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
#include "IRremote.h"

// Defining bitmasks for custom letters on the segment display
#define BM_P 0b01110011
#define BM_U 0b00111110
#define BM_G 0b00111101
#define BM_O 0b00111111

Adafruit_7segment matrix = Adafruit_7segment();

uint8_t counter = 0;

const int triggerPin   = 4;    // Trigger Switch
const int reloadPin    = 6;    // Reloading Switch
const int fireLedPin   = 3;    // IR LED
const int myAmmoLedPin = 9;    // Out of Ammo LED
const int hitLedPin    = 13;   // Hit Indicator
const int irSensorPin  = 2;    // IR Sensor
const int speakerPin   = 12;   // Piezo Speaker

// Game play variables 
int myCode             = 1;    // Unused at the moment
int maxHealth          = 3;    // Number of times you can get shot before dying
int myHealth           = maxHealth;
int maxAmmo            = 30;   // Amount of ammo returned on a reload
int ammoCount          = 30;   // Starting ammo
int deadTime           = 1000; // Number of ms you stay dead

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
    
    if (results.value == 275977667) {
      Serial.println("HIT HIT HIT");
      digitalWrite(hitLedPin, HIGH);
      // TODO: Less lame hit sound
      playTone(2500, 200);
      
      // Reduce their health and then check if they should be dead or not
      myHealth--;
      if (myHealth <= 0) {
        Serial.println("DEAD!"); 
        matrix.blinkRate(3);
        matrix.writeDigitNum(0, 'D');
        matrix.writeDigitNum(1, 'E');
        matrix.writeDigitNum(3, 'A');
        matrix.writeDigitNum(4, 'D');
        matrix.writeDisplay();
        playTone(5000, 1000);      
        delay(deadTime);
        matrix.blinkRate(0);
        matrix.print(ammoCount, DEC);
        matrix.writeDisplay();
        myHealth = maxHealth;
        Serial.println("Ready again!"); 

      } else {
        Serial.print("Health: ");
        Serial.println(myHealth); 
      }
      
      
    } else {
      Serial.println("IR Signal but not a hit");
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
