// 
// ArduNFC 
//
// Description of the project
// Developed with [embedXcode](http://embedXcode.weebly.com)
// 
// Author	 	Davide Corradini
// 				___FULLUSERNAME___
//
// Date			23/07/14 14:30
// Version		<#version#>
// 
// Copyright	© Davide Corradini, 2014
// License		<#license#>
//
// See			ReadMe.txt for references
//

// Core library for code-sense
#if defined(WIRING) // Wiring specific
#include "Wiring.h"
#elif defined(ARDUINO) // Arduino 1.0 and 1.5 specific
#include "Arduino.h"
#else // error
#error Platform not defined
#endif

// Include application, user and local libraries

#include "SPI.h"
#include "MPN532_SPI.h"
#include "MyCard.h"
#include "MNdefMessage.h"
#include <avr/wdt.h>
#define SERIAL_COMMAND_CONNECTION "connection:"
#define SERIAL_COMMAND_RECHARGE "recharge:"
#define SERIAL_COMMAND_PURCHASE "purchase:"
#define SERIAL_COMMAND_SET_DATA "set_data:"
#define SERIAL_COMMAND_GET_TIME "get_time:"
#define SERIAL_COMMAND_SET_TIME "set_time:"
#define SERIAL_RESPONSE_OK "ok;"
#define SERIAL_RESPONSE_ERROR "err;"
#define SERIAL_VALUE_REQUEST "req;"


typedef enum {S_DISCONNECTED, S_CONNECTED} SerialState;

SerialState serialState;

PN532_SPI pn532spi(SPI, 10);
MyCard nfc(pn532spi);

uint8_t ndefBuf[120];
NdefMessage message;
int messageSize;

uint8_t uid[3] = { 0x12, 0x34, 0x56 };
volatile int n = 0;
int led = 7;
int led1 = 6;
int led2 = 5;

String inputCommand = "";         // a string to hold incoming data
String inputValue = "";
boolean commandComplete = false;  // whether the string is complete
boolean valueIn = false;


ISR(WDT_vect) {
    static boolean state = false;
    if(Serial.available() > 0) {
        while(Serial.available() > 0) {
            // get the new byte:
            char inChar = (char)Serial.read();
            // add it to the inputString:
            
            if(!valueIn) {
                inputCommand += inChar;
            } else {
                inputValue += inChar;
            }
            // if the incoming character is a newline, set a flag
            // so the main loop can do something about it:
            if (inChar == ':') {
                valueIn = true;
            }
            if (inChar == ';') {
                commandComplete = true;
            }
        }
        
    }
    if (commandComplete) {
        if(inputCommand.equals(SERIAL_COMMAND_CONNECTION)) {
            if(inputValue.equals(SERIAL_RESPONSE_OK)) {
                serialState = S_CONNECTED;
                digitalWrite(led, HIGH);
            }
        } else if (inputCommand.equals("set_data:")) {
            
        } else if (inputCommand.equals(SERIAL_COMMAND_PURCHASE) && (serialState == CONNECTED)) {
            digitalWrite(led1, HIGH);
            
        } else if (inputCommand.equals(SERIAL_COMMAND_RECHARGE) && (serialState == CONNECTED)) {
            digitalWrite(led2, HIGH);
        }
        inputCommand = "";
        inputValue = "";
        valueIn = false;
        commandComplete = false;
    }
}

void watchdogTimerEnable(const byte interval) {
    noInterrupts();
    MCUSR = 0;                          // reset various flags
    WDTCSR |= 0b00011000;               // see docs, set WDCE, WDE
    WDTCSR =  0b01000000 | interval;    // set WDIE, and appropriate delay
    wdt_reset();
    interrupts();
}

//
// Brief	Setup
// Details	Define the pin the LED is connected to
//
// Add setup code 
void setup() {
    pinMode(led, OUTPUT);
    pinMode(led1, OUTPUT );
    pinMode(led2, OUTPUT);
    digitalWrite(led1, LOW);
    digitalWrite(led, LOW);
    digitalWrite(led2, LOW);
    serialState = S_DISCONNECTED;
    Serial.begin(230400);
    //Serial.println("------- Emulate Tag --------");
    //Serial.println("connection:req;");
    /*if(serialState == S_DISCONNECTED) {
        Serial.println("connection:req;");
    }*/
    message = NdefMessage();
    message.addMimeMediaRecord("application/nfcvending", "ciao");
    messageSize = message.getEncodedSize();
    if (messageSize > sizeof(ndefBuf)) {
        //Serial.println("ndefBuf is too small");
        while (1) { }
    }
    
    //Serial.print("Ndef encoded message size: ");
    //Serial.println(messageSize);
    
    message.encode(ndefBuf);
    
    // comment out this command for no ndef message
    nfc.setNdefFile(ndefBuf, messageSize);
    
    // uid must be 3 bytes!
    nfc.setUid(uid);
    
    nfc.init();
    
    
    // sleep bit patterns:
    //  16 ms: 0b000000
    //  32 ms: 0b000001
    //  64 ms: 0b000010
    //  125 ms: 0b000011
    //  250 ms: 0b000100
    //  500 ms: 0b000101
    //  1 s:  0b000110
    //  2 s: 0b000111
    //  4 s: 0b100000
    //  8 s: 0b100001
    watchdogTimerEnable(0b000101);
}

//
// Brief	Loop
// Details	Blink the LED
//
// Add loop code

void loop() {
    
    Serial.println("connection:req;");

    
    nfc.emulate();
}


