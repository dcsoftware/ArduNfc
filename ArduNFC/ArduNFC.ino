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

PN532_SPI pn532spi(SPI, 10);
MyCard nfc(pn532spi);

uint8_t ndefBuf[120];
NdefMessage message;
int messageSize;

uint8_t uid[3] = { 0x12, 0x34, 0x56 };

String inputCommand = "";         // a string to hold incoming data
String inputValue = "";
boolean commandComplete = false;  // whether the string is complete
boolean valueIn = false;


void parseCommand() {
    
}

//
// Brief	Setup
// Details	Define the pin the LED is connected to
//
// Add setup code 
void setup() {
    Serial.begin(230400);
    Serial.println("------- Emulate Tag --------");
    
    message = NdefMessage();
    message.addMimeMediaRecord("application/nfcvending", "ciao");
    messageSize = message.getEncodedSize();
    if (messageSize > sizeof(ndefBuf)) {
        Serial.println("ndefBuf is too small");
        while (1) { }
    }
    
    Serial.print("Ndef encoded message size: ");
    Serial.println(messageSize);
    
    message.encode(ndefBuf);
    
    // comment out this command for no ndef message
    nfc.setNdefFile(ndefBuf, messageSize);
    
    // uid must be 3 bytes!
    nfc.setUid(uid);
    
    nfc.init();
    
    nfc.emulate();
}

//
// Brief	Loop
// Details	Blink the LED
//
// Add loop code

void loop() {
    
    // or start emulation with timeout
    /*if(!nfc.emulate(1000)){ // timeout 1 second
     Serial.println("timed out");
     }*/
    
    // deny writing to the tag
    // nfc.setTagWriteable(false);
    
    /*if(nfc.writeOccured()){
        Serial.println("\nWrite occured !");
        uint8_t* tag_buf;
        uint16_t length;
        
        nfc.getContent(&tag_buf, &length);
        NdefMessage msg = NdefMessage(tag_buf, length);
        msg.print();
    }
    
    delay(1000);*/
    
    
    nfc.readData();
    
    if (commandComplete) {
        parseCommand();
        //Serial.println(inputString);
        // clear the string:
        inputCommand = "";
        inputValue = "";
        valueIn = false;
        commandComplete = false;
    }
}

/*
 SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
    while (Serial.available()) {
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
        if (inChar == '\n') {
            commandComplete = true;
        } 
    }
}


