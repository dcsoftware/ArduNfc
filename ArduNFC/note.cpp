//
// note.cpp 
// C++ code
// ----------------------------------
// Developed with embedXcode 
// http://embedXcode.weebly.com
//
// Project 		ArduNFC
//
// Created by 	Davide Corradini, 11/09/14 15:56
// 				Davide Corradini
//
// Copyright	© Davide Corradini, 2014
// License   	<#license#>
//
// See 			note.h and ReadMe.txt for references
//


//bool MyCard::emulate2(const uint16_t tgInitAsTargetTimeout) {
//
//    if(uidPtr != 0){  // if uid is set copy 3 bytes to nfcid1
//        memcpy(command + 4, uidPtr, 3);
//    }
//
//    if(1 != pn532.tgInitAsTarget(command,sizeof(command), tgInitAsTargetTimeout)){
//        DMSG("tgInitAsTarget failed or timed out!");
//        return false;
//    }
//
//
//
//    if(tagWriteable == false){
//        base_capability_container[14] = 0xFF;
//    }
//
//    tagWrittenByInitiator = false;
//
//
//    bool runLoop = true;
//
//    commType = NFC_COMM;
//
//    while(runLoop){
//
//        switch (commType) {
//            case NFC_COMM:
//                status = pn532.tgGetData(rwbuf, sizeof(rwbuf));
//                if(status < 0){
//                    DMSG("tgGetData timed out\n");
//                    //pn532.inRelease();
//                    //return -1;
//                }
//
//                switch (rwbuf[C_APDU_INS]) {
//                    case SELECT_FILE:
//                        switch(p1){
//                            case C_APDU_P1_SELECT_BY_ID:
//                                if(p2 != 0x0c){
//                                    DMSG("C_APDU_P2 != 0x0c\n");
//                                    setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
//                                } else if(lc == 2 && rwbuf[C_APDU_DATA] == 0xE1 && (rwbuf[C_APDU_DATA+1] == 0x03 || rwbuf[C_APDU_DATA+1] == 0x04)){
//                                    setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
//                                    if(rwbuf[C_APDU_DATA+1] == 0x03){
//                                        currentFile = CC;
//                                    } else if(rwbuf[C_APDU_DATA+1] == 0x04){
//                                        currentFile = NDEF;
//                                    }
//                                } else {
//                                    setResponse(TAG_NOT_FOUND, rwbuf, &sendlen);
//                                }
//                                break;
//                            case C_APDU_P1_SELECT_BY_NAME:
//                                if(0 == memcmp(ndef_tag_application_name_v2, rwbuf + C_APDU_P2, sizeof(ndef_tag_application_name_v2))){
//                                    cardState = CONNECTED;
//                                    setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
//                                } else if (0 == memcmp(ndef_tag_application_name_priv, rwbuf + C_APDU_P2, sizeof(ndef_tag_application_name_priv))){
//                                    DMSG("\nOK");
//                                    //if(serialState == S_DISCONNECTED) {
//                                    watchdogTimerEnable(0b000011);
//                                    Serial.println("connection:req;");
//                                    //}
//                                    setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
//                                } else {
//                                    DMSG("function not supported\n");
//                                    setResponse(FUNCTION_NOT_SUPPORTED, rwbuf, &sendlen);
//                                }
//                                break;
//                        }
//                        break;
//                    case READ_BINARY:
//                        switch(currentFile){
//                            case NONE:
//                                setResponse(TAG_NOT_FOUND, rwbuf, &sendlen);
//                                break;
//                            case CC:
//                                if( p1p2_length > NDEF_MAX_LENGTH){
//                                    setResponse(END_OF_FILE_BEFORE_REACHED_LE_BYTES, rwbuf, &sendlen);
//                                }else {
//                                    memcpy(rwbuf,base_capability_container + p1p2_length, lc);
//                                    setResponse(COMMAND_COMPLETE, rwbuf + lc, &sendlen, lc);
//                                }
//                                break;
//                            case NDEF:
//                                if( p1p2_length > NDEF_MAX_LENGTH){
//                                    setResponse(END_OF_FILE_BEFORE_REACHED_LE_BYTES, rwbuf, &sendlen);
//                                }else {
//                                    memcpy(rwbuf, ndef_file + p1p2_length, lc);
//                                    setResponse(COMMAND_COMPLETE, rwbuf + lc, &sendlen, lc);
//                                }
//                                break;
//                        }
//                        break;
//                    case UPDATE_BINARY:
//                        if(!tagWriteable){
//                            setResponse(FUNCTION_NOT_SUPPORTED, rwbuf, &sendlen);
//                        } else{
//                            if( p1p2_length > NDEF_MAX_LENGTH){
//                                setResponse(MEMORY_FAILURE, rwbuf, &sendlen);
//                            }
//                            else{
//                                memcpy(ndef_file + p1p2_length, rwbuf + C_APDU_DATA, lc);
//                                setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
//                                tagWrittenByInitiator = true;
//
//                                uint16_t ndef_length = (ndef_file[0] << 8) + ndef_file[1];
//                                if ((ndef_length > 0) && (updateNdefCallback != 0)) {
//                                    updateNdefCallback(ndef_file + 2, ndef_length);
//                                }
//                            }
//                        }
//                        break;
//                    case AUTHENTICATE:
//                        if((p1 == 0x00) && (p2 == 0x00)) {
//                            //DMSG("\nAuthenticating... ");
//                            for (int i = 0; i <= lc; i++) {
//                                //DMSG_HEX(rwbuf[C_APDU_DATA + i]);
//                                otpReceived += rwbuf[C_APDU_DATA + i];
//                                DMSG_WRT(rwbuf[C_APDU_DATA + i]);
//                            }
//                            Serial.println("get_time:req");
//                            commType = SERIAL_COMM;
//
////                            cardState = WAITING_SERIAL;
////                            setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
//
//                        }
//                        break;
//                    case LOG_IN:
//                        if((p1 == 0x00) && (p2 == 0x00)) {
//                            /*DMSG("\nLoggin in... ");
//                             for (int i = 0; i <= lc; i++) {
//                             Serial.write(rwbuf[C_APDU_DATA + i]);
//                             }*/
//                            cardState = AUTHENTICATED;
//                            setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
//
//                        }
//                        break;
//                    case READING_STATUS:
//                        if((p1 == 0x00) && (p2 == 0x00)) {
//
//                            if (reading = 0 ) {
//                                DMSG("\nReading status");
//                                reading++;
//                            }
//
//                            DMSG(".");
//                            /*if(Serial.available() > 0) {
//                             String command = Serial.readStringUntil(*comTerminator);
//
//                             if (command.equals("R")) {
//                             setResponse(STATUS_RECHARGED, rwbuf, &sendlen);
//                             } else if (command.equals("P")) {
//                             setResponse(STATUS_PURCHASE, rwbuf, &sendlen);
//                             }
//
//                             } else {
//
//
//                             state = AUTHENTICATED;
//                             setResponse(STATUS_WAITING, rwbuf, &sendlen);
//                             }*/
//                            cardState = AUTHENTICATED;
//                            setResponse(STATUS_WAITING, rwbuf, &sendlen);
//
//                        }
//                        break;
//                    case UPDATE_CREDIT:
//                        if((p1 == 0x00) && (p2 == 0x00)) {
//                            DMSG("\nVerifying: ");
//
//
//                            cardState = AUTHENTICATED;
//                            setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
//
//                        }
//                        break;
//                    default:
//                        DMSG("Command not supported!");
//                        DMSG_HEX(rwbuf[C_APDU_INS]);
//                        DMSG("\n");
//                        setResponse(FUNCTION_NOT_SUPPORTED, rwbuf, &sendlen);
//                }
//
//
//                break;
//            case SERIAL_COMM:
//                String response = "";
//                static boolean state = false;
//                if(Serial.available() > 0) {
//                    while(Serial.available() > 0) {
//                        // get the new byte:
//                        char inChar = (char)Serial.read();
//                        // add it to the inputString:
//
//                        if(!valueIn) {
//                            inputCommand += inChar;
//                        } else {
//                            inputValue += inChar;
//                        }
//                        // if the incoming character is a newline, set a flag
//                        // so the main loop can do something about it:
//                        if (inChar == ':') {
//                            valueIn = true;
//                        }
//                        if (inChar == ';') {
//                            commandComplete = true;
//                        }
//                    }
//
//                }
//                if (commandComplete) {
//                    if(inputCommand.equals(SERIAL_COMMAND_CONNECTION)) {
//                        if(inputValue.equals(SERIAL_RESPONSE_OK)) {
//                            serialState = S_CONNECTED;
//                            digitalWrite(led, HIGH);
//                        }
//                    } else if (inputCommand.equals("set_data:")) {
//
//                    } else if (inputCommand.equals(SERIAL_COMMAND_PURCHASE) && (serialState == CONNECTED)) {
//                        //digitalWrite(led1, HIGH);
//                        Serial.println(inputCommand.concat(SERIAL_RESPONSE_OK));
//
//                    } else if (inputCommand.equals(SERIAL_COMMAND_RECHARGE) && (serialState == CONNECTED)) {
//                        digitalWrite(led2, HIGH);
//                        Serial.println(inputCommand.concat(SERIAL_RESPONSE_OK));
//                    } else if (inputCommand.equals(SERIAL_COMMAND_GET_TIME) && (serialState == CONNECTED)) {
//                        updateTime(true, inputValue.toInt());
//                    }
//                    inputCommand = "";
//                    inputValue = "";
//                    valueIn = false;
//                    commandComplete = false;
//                }
//
//                if(timeRead) {
//                    char* newCode = totp.getCode(epoch);
//                    if(strcmp(code, newCode) != 0) {
//                        strcpy(code, newCode);
//                        Serial.println(code);
//                        digitalWrite(led1, HIGH);
//                        timeRead = false;
//                    }
//                }
//
//
//                setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
//                commType = NFC_COMM;
//                break;
//        }
//        status = pn532.tgSetData(rwbuf, sendlen);
//        if(status < 0){
//            DMSG("tgSetData failed\n!");
//            DMSG("\n In Release 1");
//            pn532.inRelease();
//            return true;
//        }
//    }
//
//}

//case AUTHENTICATE:
//if((p1 == 0x00) && (p2 == 0x00)) {
//    //DMSG("\nAuthenticating... ");
//    for (int i = 0; i <= lc; i++) {
//        //DMSG_HEX(rwbuf[C_APDU_DATA + i]);
//        otpReceived += rwbuf[C_APDU_DATA + i];
//        DMSG_WRT(rwbuf[C_APDU_DATA + i]);
//    }
//    
//    Serial.println("get_time:req");
//    
//    //                    while (!timeRead) {
//    //                        delay(10);
//    //                    }
//    //                    digitalWrite(led2, HIGH);
//    //
//    //
//    //                    if (timeRead) {
//    //                        char* newCode = totp.getCode(epoch);
//    //                        if(strcmp(code, newCode) != 0) {
//    //                            strcpy(code, newCode);
//    //                        }
//    //
//    //                        if(strcmp(code, otpReceived) == 0) {
//    //                            digitalWrite(led2, HIGH);
//    //                        }
//    //                    }
//    
//    cardState = WAITING_SERIAL;
//    setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
//    
//}

//void updateTime(bool updated, long time) {
//    epoch = time;
//    timeRead = updated;
//    digitalWrite(led1, HIGH);
//}
//ISR(WDT_vect) {
//    commType = SERIAL_COMM;
//    String response = "";
//    static boolean state = false;
//    if(Serial.available() > 0) {
//        while(Serial.available() > 0) {
//            // get the new byte:
//            char inChar = (char)Serial.read();
//            // add it to the inputString:
//            
//            if(!valueIn) {
//                inputCommand += inChar;
//            } else {
//                inputValue += inChar;
//            }
//            // if the incoming character is a newline, set a flag
//            // so the main loop can do something about it:
//            if (inChar == ':') {
//                valueIn = true;
//            }
//            if (inChar == ';') {
//                commandComplete = true;
//                Serial.println(inputCommand);
//                
//                
//            }
//        }
//        
//    }
//    if (commandComplete) {
//        if(inputCommand.equals(SERIAL_COMMAND_CONNECTION)) {
//            if(inputValue.equals(SERIAL_RESPONSE_OK)) {
//                serialState = S_CONNECTED;
//                digitalWrite(led, HIGH);
//            }
//        } else if (inputCommand.equals("set_data:")) {
//            
//        } else if (inputCommand.equals(SERIAL_COMMAND_PURCHASE) && (serialState == CONNECTED)) {
//            digitalWrite(led1, HIGH);
//            Serial.println(inputCommand.concat(SERIAL_RESPONSE_OK));
//            
//        } else if (inputCommand.equals(SERIAL_COMMAND_RECHARGE) && (serialState == CONNECTED)) {
//            digitalWrite(led2, HIGH);
//            Serial.println(inputCommand.concat(SERIAL_RESPONSE_OK));
//        } else if (inputCommand.equals(SERIAL_COMMAND_GET_TIME) && (serialState == CONNECTED)) {
//            updateTime(true, inputValue.toInt());
//        }
//        inputCommand = "";
//        inputValue = "";
//        valueIn = false;
//        commandComplete = false;
//    }
//}
//
//void watchdogTimerEnable(const byte interval) {
//    noInterrupts();
//    MCUSR = 0;                          // reset various flags
//    WDTCSR |= 0b00011000;               // see docs, set WDCE, WDE
//    WDTCSR =  0b01000000 | interval;    // set WDIE, and appropriate delay
//    wdt_reset();
//    interrupts();
//}
