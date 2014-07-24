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
#elif defined(MAPLE_IDE) // Maple specific
#include "WProgram.h"   
#elif defined(MICRODUINO) // Microduino specific
#include "Arduino.h"
#elif defined(MPIDE) // chipKIT specific
#include "WProgram.h"
#elif defined(DIGISPARK) // Digispark specific
#include "Arduino.h"
#elif defined(ENERGIA) // LaunchPad MSP430, Stellaris and Tiva, Experimeter Board FR5739 specific
#include "Energia.h"
#elif defined(TEENSYDUINO) // Teensy specific
#include "Arduino.h"
#elif defined(ARDUINO) // Arduino 1.0 and 1.5 specific
#include "Arduino.h"
#else // error
#error Platform not defined
#endif

// Include application, user and local libraries

#include "SPI.h"
#include "MPN532_SPI.h"
//#include "memulatetag.h"
#include "MyCard.h"
#include "MNdefMessage.h"

PN532_SPI pn532spi(SPI, 10);
//EmulateTag nfc(pn532spi);
MyCard nfc(pn532spi);

uint8_t ndefBuf[120];
NdefMessage message;
int messageSize;

uint8_t uid[3] = { 0x12, 0x34, 0x56 };


//
// Brief	Setup
// Details	Define the pin the LED is connected to
//
// Add setup code 
void setup() {
    Serial.begin(115200);
    Serial.println("------- Emulate Tag --------");
    
    message = NdefMessage();
    message.addUriRecord("http://www.seeedstudio.com");
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
}

//
// Brief	Loop
// Details	Blink the LED
//
// Add loop code 
void loop() {
    // uncomment for overriding ndef in case a write to this tag occured
    //nfc.setNdefFile(ndefBuf, messageSize);
    
    // start emulation (blocks)
    nfc.emulate();
    
    // or start emulation with timeout
    /*if(!nfc.emulate(1000)){ // timeout 1 second
     Serial.println("timed out");
     }*/
    
    // deny writing to the tag
    // nfc.setTagWriteable(false);
    
    if(nfc.writeOccured()){
        Serial.println("\nWrite occured !");
        uint8_t* tag_buf;
        uint16_t length;
        
        nfc.getContent(&tag_buf, &length);
        NdefMessage msg = NdefMessage(tag_buf, length);
        msg.print();
    }
    
    delay(1000);
}
