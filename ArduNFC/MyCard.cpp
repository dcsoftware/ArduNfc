/**************************************************************************/
/*!
 @file     emulatetag.cpp
 @author   Armin Wieser
 @license  BSD
 */
/**************************************************************************/

#include "MyCard.h"
#include "MPN532_debug.h"
#include "MNdefMessage.h"
#include <string.h>
#include <Serial.h>
#include "sha1.h"
#include "TOTP.h"
#include <avr/wdt.h>
#include <stdlib.h>
#define MAX_TGREAD


// Command APDU
#define C_APDU_CLA   0
#define C_APDU_INS   1 // instruction
#define C_APDU_P1    2 // parameter 1
#define C_APDU_P2    3 // parameter 2
#define C_APDU_LC    4 // length command
#define C_APDU_DATA  5 // data

#define C_APDU_P1_SELECT_BY_ID   0x00
#define C_APDU_P1_SELECT_BY_NAME 0x04

// Response APDU
#define R_APDU_SW1_COMMAND_COMPLETE 0x90
#define R_APDU_SW2_COMMAND_COMPLETE 0x00

#define R_APDU_SW1_NDEF_TAG_NOT_FOUND 0x6a
#define R_APDU_SW2_NDEF_TAG_NOT_FOUND 0x82

#define R_APDU_SW1_FUNCTION_NOT_SUPPORTED 0x6A
#define R_APDU_SW2_FUNCTION_NOT_SUPPORTED 0x81

#define R_APDU_SW1_MEMORY_FAILURE 0x65
#define R_APDU_SW2_MEMORY_FAILURE 0x81

#define R_APDU_SW1_END_OF_FILE_BEFORE_REACHED_LE_BYTES 0x62
#define R_APDU_SW2_END_OF_FILE_BEFORE_REACHED_LE_BYTES 0x82
#define R_PRIV_ADDRESS_BYTE1 0xAA
#define R_PRIV_ADDRESS_BYTE2 0xAA
#define R_SW1_ERROR_AUTH 0xB1
#define R_SW2_ERROR_AUTH 0xB2
#define R_SW1_STATUS_WAITING 0x22
#define R_SW2_STATUS_WAITING 0x33
#define R_SW1_STATUS_RECHARGED 0x33
#define R_SW2_STATUS_RECHARGED 0x44
#define R_SW1_STATUS_PURCHASE 0x44
#define R_SW2_STATUS_PURCHASE 0x55
#define R_SW1_STATUS_DATA_UPDATED 0x55
#define R_SW2_STATUS_DATA_UPDATED 0x66

// ISO7816-4 commands
#define SELECT_FILE 0xA4
#define READ_BINARY 0xB0
#define UPDATE_BINARY 0xD6
#define AUTHENTICATE 0x20
#define LOG_IN 0x30
#define READING_STATUS 0x40
#define UPDATE_CREDIT 0x50

#define S_COM_RECHARGE 0x52
#define S_COM_PURCHASE 0x50

#define SERIAL_COMMAND_CONNECTION "connection:"
#define SERIAL_COMMAND_RECHARGE "recharge:"
#define SERIAL_COMMAND_PURCHASE "purchase:"
#define SERIAL_COMMAND_SET_DATA "set_data:"
#define SERIAL_COMMAND_GET_TIME "get_time:"
#define SERIAL_COMMAND_SET_TIME "set_time:"
#define SERIAL_COMMAND_LOG "log:";
#define SERIAL_RESPONSE_OK "ok;"
#define SERIAL_RESPONSE_ERROR "err;"
#define SERIAL_VALUE_REQUEST "req;"
#define SERIAL_COMMAND_START "<"
#define SERIAL_COMMAND_END ">"

typedef enum { NONE, CC, NDEF} tag_file;   // CC ... Compatibility Container

typedef enum {S_DISCONNECTED, S_CONNECTED} SerialState;

typedef enum {NFC_COMM, SERIAL_COMM} CommType;



SerialState serialState;
CardState cardState;
CommType commType = NFC_COMM;

int reading = 0;


String userId;
String userCredit;
uint8_t secretK[] = "ABCDEFGHIJ"; //{0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a};
int intCount = 0;
bool timeRead = false;
long epoch = 0;
//char code[6];
int led = 7;
int led1 = 6;
int led2 = 5;
String inputCommand = "";         // a string to hold incoming data
String inputValue = "";
String otpIn;
boolean commandComplete = false;  // whether the string is complete
boolean valueIn = false;
boolean serialInt = false;
char* otpReceived = "";
char otp[10];

TOTP totp = TOTP(secretK, 10);

void verifyOtpCode(String input) {
    char* newCode;
    char code[10];
    char prevCode[10];
    char nextCode[10];
    
    String a = input.substring(0, input.length() - 5);
    char buf[20];
    a.concat('0');
    a.toCharArray(buf, sizeof(buf));
    buf[sizeof(buf) - 1] = 0;
    epoch = atol(buf);
    /*Serial.print("log: epoch = ");
     Serial.print(epoch);
     Serial.print(" - timestep: ");
     Serial.print(timestep);
     Serial.println(" ;");*/
    
    
    newCode = totp.getCode(epoch);
    strcpy(code, newCode);
    newCode = totp.getCode(epoch - 10);
    strcpy(prevCode, newCode);
    newCode = totp.getCode(epoch + 10);
    strcpy(nextCode, newCode);

    
    int f = strcmp(code, otp);
    Serial.print("log: generated otp = ");
    Serial.print(prevCode);
    Serial.print(" ");
    Serial.print(code);
    Serial.print(" ");
    Serial.print(nextCode);
    Serial.print(" ");
    Serial.print(" received otp = ");
    Serial.print(otp);
    Serial.print(" - compare = ");
    Serial.print(f);
    Serial.println(" ;");
    
    delay(100);
    
    
    
    if(strcmp(prevCode, otp) == 0) {
        cardState = AUTHENTICATED;
        Serial.println("log: AUTHENTICATION OK con prevCode;");
        digitalWrite(led2, HIGH);
    } else if(strcmp(code, otp) == 0) {
        cardState = AUTHENTICATED;
        Serial.println("log: AUTHENTICATION OK con code;");
        digitalWrite(led2, HIGH);
    } else if(strcmp(nextCode, otp) == 0) {
        cardState = AUTHENTICATED;
        Serial.println("log: AUTHENTICATION OK con nextCode;");
        digitalWrite(led2, HIGH);
    } else {
        cardState = ERROR_AUTH;
        Serial.println("log: AUTHENTICATION ERROR;");

    }
}

void readCommand() {
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
                break;
            }
        }
        
    }
    if (commandComplete) {
        //Serial.println("log:" + inputCommand + "-" + inputValue);
        if(inputCommand.equals(SERIAL_COMMAND_CONNECTION)) {
            if(inputValue.equals(SERIAL_RESPONSE_OK)) {
                serialState = S_CONNECTED;
                digitalWrite(led, HIGH);
            }
        } else if (inputCommand.equals("set_data:")) {
            if(inputValue.equals(SERIAL_RESPONSE_OK)) {
                
            }
        } else if (inputCommand.equals(SERIAL_COMMAND_PURCHASE) && (serialState == S_CONNECTED)) {
            //digitalWrite(led1, HIGH);
            Serial.println(inputCommand.concat(SERIAL_RESPONSE_OK));
            cardState = PURCHASE;
            
        } else if (inputCommand.equals(SERIAL_COMMAND_RECHARGE) && (serialState == S_CONNECTED)) {
            //digitalWrite(led2, HIGH);
            cardState = RECHARGE;
            Serial.println(inputCommand.concat(SERIAL_RESPONSE_OK));
        } else if (inputCommand.equals(SERIAL_COMMAND_GET_TIME) && (serialState == S_CONNECTED)) {
            
            verifyOtpCode(inputValue);

        }
        inputCommand = "";
        inputValue = "";
        valueIn = false;
        commandComplete = false;
    }
}

void waitingSerial() {
    while (!serialInt) {
        //digitalWrite(led2, !digitalRead(led2));
        delay(100);
    }
    
    readCommand();
}

ISR(WDT_vect) {
    serialInt = false;
    commType = SERIAL_COMM;
    static boolean state = false;
    if(Serial.available() > 0) {
        serialInt = true;
        //digitalWrite(led1, HIGH);
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


bool MyCard::init(){
    pn532.begin();
    return pn532.SAMConfig();
}

void MyCard::setNdefFile(const uint8_t* ndef, const int16_t ndefLength){
    if(ndefLength >  (NDEF_MAX_LENGTH -2)){
        DMSG("ndef file too large (> NDEF_MAX_LENGHT -2) - aborting");
        return;
    }
    
    ndef_file[0] = ndefLength >> 8;
    ndef_file[1] = ndefLength & 0xFF;
    memcpy(ndef_file+2, ndef, ndefLength);
}

void MyCard::setUid(uint8_t* uid){
    uidPtr = uid;
}

bool MyCard::emulate(const uint16_t tgInitAsTargetTimeout){
    
    pinMode(led, OUTPUT);
    pinMode(led1, OUTPUT );
    pinMode(led2, OUTPUT);
    digitalWrite(led1, LOW);
    digitalWrite(led, LOW);
    digitalWrite(led2, LOW);
    userCredit = "0.0";
    userId = "";
    watchdogTimerEnable(0b000100);
    
    const uint8_t ndef_tag_application_name_v2[] = {0, 0x7, 0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01 };
    const uint8_t ndef_tag_application_name_priv[] = {0, 0x7, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34};
    
    uint8_t command[] = {
        PN532_COMMAND_TGINITASTARGET,
        5,                  // MODE: PICC only, Passive only
        
        0x04, 0x00,         // SENS_RES
        0x00, 0x00, 0x00,   // NFCID1
        0x20,               // SEL_RES
        
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,   // FeliCaParams
        0,0,
        
        0,0,0,0,0,0,0,0,0,0, // NFCID3t
        
        0, // length of general bytes
        0  // length of historical bytes
    };
    
    
    if(uidPtr != 0){  // if uid is set copy 3 bytes to nfcid1
        memcpy(command + 4, uidPtr, 3);
    }
    
    if(1 != pn532.tgInitAsTarget(command,sizeof(command), tgInitAsTargetTimeout)){
        DMSG("tgInitAsTarget failed or timed out!");
        return false;
    }
    
    uint8_t base_capability_container[] = {
        0, 0x0F,    //CC length
        0x20,       //Mapping Version ---> version 2.0
        0, 0x54,    //Max data read
        0, 0xFF,    //Max data write
        0x04,       // T
        0x06,       // L
        0xE1, 0x04, // File identifier
        ((NDEF_MAX_LENGTH & 0xFF00) >> 8), (NDEF_MAX_LENGTH & 0xFF), // maximum NDEF file size
        0x00,       // read access 0x0 = granted
        0x00        // write access 0x0 = granted | 0xFF = deny
    };
    
    if(tagWriteable == false){
        base_capability_container[14] = 0xFF;
    }
    
    tagWrittenByInitiator = false;
    
    
    uint8_t rwbuf[128];
    uint8_t sendlen;
    int16_t status;
    tag_file currentFile = NONE;
    uint16_t cc_size = sizeof(base_capability_container);
    bool runLoop = true;
    
    while(runLoop){

        status = pn532.tgGetData(rwbuf, sizeof(rwbuf));
        if(status < 0){
            DMSG("tgGetData timed out\n");
            //pn532.inRelease();
            //return -1;
        }
        
        uint8_t p1 = rwbuf[C_APDU_P1];
        uint8_t p2 = rwbuf[C_APDU_P2];
        uint8_t lc = rwbuf[C_APDU_LC];
        uint16_t p1p2_length = ((int16_t) p1 << 8) + p2;

        
        switch(rwbuf[C_APDU_INS]){
            case SELECT_FILE:
                switch(p1){
                    case C_APDU_P1_SELECT_BY_ID:
                        if(p2 != 0x0c){
                            DMSG("C_APDU_P2 != 0x0c\n");
                            setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
                        } else if(lc == 2 && rwbuf[C_APDU_DATA] == 0xE1 && (rwbuf[C_APDU_DATA+1] == 0x03 || rwbuf[C_APDU_DATA+1] == 0x04)){
                            setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
                            if(rwbuf[C_APDU_DATA+1] == 0x03){
                                currentFile = CC;
                            } else if(rwbuf[C_APDU_DATA+1] == 0x04){
                                currentFile = NDEF;
                            }
                        } else {
                            setResponse(TAG_NOT_FOUND, rwbuf, &sendlen);
                        }
                        break;
                    case C_APDU_P1_SELECT_BY_NAME:
                        if(0 == memcmp(ndef_tag_application_name_v2, rwbuf + C_APDU_P2, sizeof(ndef_tag_application_name_v2))){
                            cardState = CONNECTED;
                            setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
                        } else if (0 == memcmp(ndef_tag_application_name_priv, rwbuf + C_APDU_P2, sizeof(ndef_tag_application_name_priv))){
                            DMSG("\nOK");
                            Serial.println("connection:req;");
                            //delay(500);
                            waitingSerial();
                            setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
                            
                        } else {
                            DMSG("function not supported\n");
                            setResponse(FUNCTION_NOT_SUPPORTED, rwbuf, &sendlen);
                        }
                        break;
                }
                break;
            case READ_BINARY:
                switch(currentFile){
                    case NONE:
                        setResponse(TAG_NOT_FOUND, rwbuf, &sendlen);
                        break;
                    case CC:
                        if( p1p2_length > NDEF_MAX_LENGTH){
                            setResponse(END_OF_FILE_BEFORE_REACHED_LE_BYTES, rwbuf, &sendlen);
                        }else {
                            memcpy(rwbuf,base_capability_container + p1p2_length, lc);
                            setResponse(COMMAND_COMPLETE, rwbuf + lc, &sendlen, lc);
                        }
                        break;
                    case NDEF:
                        if( p1p2_length > NDEF_MAX_LENGTH){
                            setResponse(END_OF_FILE_BEFORE_REACHED_LE_BYTES, rwbuf, &sendlen);
                        }else {
                            memcpy(rwbuf, ndef_file + p1p2_length, lc);
                            setResponse(COMMAND_COMPLETE, rwbuf + lc, &sendlen, lc);
                        }
                        break;
                }
                break;
            case UPDATE_BINARY:
                if(!tagWriteable){
                    setResponse(FUNCTION_NOT_SUPPORTED, rwbuf, &sendlen);
                } else{
                    if( p1p2_length > NDEF_MAX_LENGTH){
                        setResponse(MEMORY_FAILURE, rwbuf, &sendlen);
                    }
                    else{
                        memcpy(ndef_file + p1p2_length, rwbuf + C_APDU_DATA, lc);
                        setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
                        tagWrittenByInitiator = true;
                        
                        uint16_t ndef_length = (ndef_file[0] << 8) + ndef_file[1];
                        if ((ndef_length > 0) && (updateNdefCallback != 0)) {
                            updateNdefCallback(ndef_file + 2, ndef_length);
                        }
                    }
                }
                break;
            case AUTHENTICATE:
                if((p1 == 0x00) && (p2 == 0x00)) {
                    //DMSG("\nAuthenticating... ");
                    for (int i = 0; i <= lc; i++) {
                        //DMSG_HEX(rwbuf[C_APDU_DATA + i]);
                        otp[i] = rwbuf[C_APDU_DATA + i];
                        //DMSG_WRT(rwbuf[C_APDU_DATA + i]);
                    }
                    
                    /*otpIn = otp;
                    Serial.println("log: OTP received = " + otpIn + " ;");
                    delay(200);*/


                    Serial.println("get_time:req;");
                    delay(50);  //se lo tolgo si impalla...

                    waitingSerial();
                    
                    switch (cardState) {
                        case AUTHENTICATED:
                            setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
                            break;
                        case ERROR_AUTH:
                            setResponse(AUTH_ERROR, rwbuf, &sendlen);
                            break;
                    }
                    
                    
                }
                break;
            case LOG_IN:
                if((p1 == 0x00) && (p2 == 0x00)) {
                    DMSG("\nLoggin in... ");
                    char string[50];
                    for (int i = 0; i <= lc; i++) {
                        string[i] = (char)rwbuf[C_APDU_DATA + i];
                        //Serial.write(rwbuf[C_APDU_DATA + i]);
                    }
                    string[lc] = '\0';
                    String s = string;
                    int x = s.indexOf(',');
                    userId = s.substring(0, x -1);
                    userCredit = s.substring(x + 1, s.length() - 1);
                    
                    Serial.println("set_data:" + userCredit + ';');
                    
                    waitingSerial();
                    
                    setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
                    
                }
                break;
            case READING_STATUS:
                if((p1 == 0x00) && (p2 == 0x00)) {
                    
                    if (reading = 0 ) {
                        DMSG("\nReading status");
                        reading++;
                    }
                    
                    DMSG(".");
                    /*if(Serial.available() > 0) {
                        String command = Serial.readStringUntil(*comTerminator);
                        
                        if (command.equals("R")) {
                            setResponse(STATUS_RECHARGED, rwbuf, &sendlen);
                        } else if (command.equals("P")) {
                            setResponse(STATUS_PURCHASE, rwbuf, &sendlen);
                        }
                        
                    } else {
                    
                    
                    state = AUTHENTICATED;
                    setResponse(STATUS_WAITING, rwbuf, &sendlen);
                    }*/
                    cardState = AUTHENTICATED;
                    setResponse(STATUS_WAITING, rwbuf, &sendlen);
                    
                }
                break;
            case UPDATE_CREDIT:
                if((p1 == 0x00) && (p2 == 0x00)) {
                    DMSG("\nVerifying: ");

                    
                    cardState = AUTHENTICATED;
                    setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
                    
                }
                break;
            default:
                DMSG("Command not supported!");
                DMSG_HEX(rwbuf[C_APDU_INS]);
                DMSG("\n");
                setResponse(FUNCTION_NOT_SUPPORTED, rwbuf, &sendlen);
        }
        status = pn532.tgSetData(rwbuf, sendlen);
        if(status < 0){
            DMSG("tgSetData failed\n!");
            DMSG("\n In Release 1");
            pn532.inRelease();
            return true;
        }

        
    }
    DMSG("\nIn Release 2");
    pn532.inRelease();
    return true;
}


void MyCard::setResponse(responseCommand cmd, uint8_t* buf, uint8_t* sendlen, uint8_t sendlenOffset){
    switch(cmd){
        case COMMAND_COMPLETE:
            buf[0] = R_APDU_SW1_COMMAND_COMPLETE;
            buf[1] = R_APDU_SW2_COMMAND_COMPLETE;
            *sendlen = 2 + sendlenOffset;
            break;
        case TAG_NOT_FOUND:
            buf[0] = R_APDU_SW1_NDEF_TAG_NOT_FOUND;
            buf[1] = R_APDU_SW2_NDEF_TAG_NOT_FOUND;
            *sendlen = 2;
            break;
        case FUNCTION_NOT_SUPPORTED:
            buf[0] = R_APDU_SW1_FUNCTION_NOT_SUPPORTED;
            buf[1] = R_APDU_SW2_FUNCTION_NOT_SUPPORTED;
            *sendlen = 2;
            break;
        case MEMORY_FAILURE:
            buf[0] = R_APDU_SW1_MEMORY_FAILURE;
            buf[1] = R_APDU_SW2_MEMORY_FAILURE;
            *sendlen = 2;
            break;
        case END_OF_FILE_BEFORE_REACHED_LE_BYTES:
            buf[0] = R_APDU_SW1_END_OF_FILE_BEFORE_REACHED_LE_BYTES;
            buf[1] = R_APDU_SW2_END_OF_FILE_BEFORE_REACHED_LE_BYTES;
            *sendlen= 2;
            break;
        case STATUS_WAITING:
            buf[0] = R_SW1_STATUS_WAITING;
            buf[1] = R_SW2_STATUS_WAITING;
            *sendlen= 2;
            break;
        case STATUS_RECHARGED:
            buf[0] = R_SW1_STATUS_RECHARGED;
            buf[1] = R_SW2_STATUS_RECHARGED;
            buf[2] = 0x01;
            *sendlen= 2;
            break;
        case STATUS_PURCHASE:
            buf[0] = R_SW1_STATUS_PURCHASE;
            buf[1] = R_SW2_STATUS_PURCHASE;
            *sendlen= 2;
            break;
        case STATUS_DATA_UPDATED:
            buf[0] = R_SW1_STATUS_DATA_UPDATED;
            buf[1] = R_SW2_STATUS_DATA_UPDATED;
            *sendlen= 2;
            break;
        case AUTH_ERROR:
            buf[0] = R_SW1_ERROR_AUTH;
            buf[1] = R_SW2_ERROR_AUTH;
            *sendlen= 2;
            break;
    }
}

