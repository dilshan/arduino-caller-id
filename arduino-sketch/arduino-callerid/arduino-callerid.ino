/*****************************************************************************
 Copyright 2021 Dilshan R Jayakody.
 
 Permission is hereby granted, free of charge, to any person obtaining 
 a copy of this software and associated documentation files (the "Software"), 
 to deal in the Software without restriction, including without limitation 
 the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 and/or sell copies of the Software, and to permit persons to whom the 
 Software is furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included 
 in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
 OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 OTHER DEALINGS IN THE SOFTWARE.
*****************************************************************************/

#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

// Create callerID serial interface on pin 10 (RX) and 11 (TX).
#define CALLER_ID_RX  10
#define CaLLER_ID_TX  11

#define LCD_RS      2
#define LCD_EN      3
#define LCD_DATA4   4
#define LCD_DATA5   5
#define LCD_DATA6   6
#define LCD_DATA7   7

#define LCD_BACKLED         12
#define LCD_CALL_ALERT_LED  13

#define RING_DET_PIN  8
#define CLI_PWDN_PIN  9

#define MDMF_HEADER   0x80
#define MDMF_PARAM_TIME   0x01
#define MDMF_PARAM_CID    0x02
#define MDMF_PARAM_NAME   0x07 

#define EEPROM_NUM_OFFSET   1
#define EEPROM_TIME_OFFSET  20

enum CID_STATE {CID_IDLE, CID_SYNC, CID_PACKET, CID_MESSAGE, CID_END};
enum CID_MSG_STATE {CIDMSG_HEADER, CIDMSG_LEN, CIDMSG_DATA};

SoftwareSerial callerID(CALLER_ID_RX, CaLLER_ID_TX);
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_DATA4, LCD_DATA5, LCD_DATA6, LCD_DATA7);

enum CID_STATE cidState;
enum CID_MSG_STATE cidMsg;

unsigned char tempCount, cidData, packetLen, currLen;
unsigned char msgType, msgLen, msgCurrPos;
unsigned long delayStart, watchdogDelay, recallTimeout;
unsigned char lineIdleCounter, watchdogSec;
unsigned char buttonState, tempInfoPos, recall;

char msgData[16];

char tBufferNumber[16];
char tBufferDateTime[9];

void programLastNumber()
{
  unsigned char memPos;

  // Write last caller ID information into the temporary buffer.
  // This temporary buffer is used to avoid sync issues in serial stream (due to slow EEPROM write operations). 
  for(memPos = 0; memPos < 16; memPos++)
  {
    tBufferNumber[memPos] = msgData[memPos];
  }
}

void programLastCallTime()
{
  unsigned char memPos;

  // Write date/time related to the last call into the temporary buffer.
  // This temporary buffer is used to avoid sync issues in serial stream (due to slow EEPROM write operations). 
  for(memPos = 0; memPos < 9; memPos++)
  {
    tBufferDateTime[memPos] = msgData[memPos];
  }
}

void saveCallInfoToEEPROM()
{
  unsigned char memPos;

  // Save caller ID information to the EEPROM.
  for(memPos = 0; memPos < 16; memPos++)
  {
    EEPROM.write(EEPROM_NUM_OFFSET + memPos, tBufferNumber[memPos]);
  }

  // Save date/time information to the EEPROM.
  for(memPos = 0; memPos < 9; memPos++)
  {
    EEPROM.write(EEPROM_TIME_OFFSET + memPos, tBufferDateTime[memPos]);
  }
}

void printCallerIDTime()
{
  unsigned char month = ((msgData[0] - 0x30) * 10) + (msgData[1] - 0x30);  
  char valStr[3];
  String monthStr = "Jan";

  // Print month on LCD.
  lcd.setCursor(0, 1);
  switch(month)
  {
    case 2:
      monthStr = "Feb";
      break;
    case 3:
      monthStr = "Mar";
      break;
    case 4:
      monthStr = "Apr";
      break;
    case 5:
      monthStr = "May";
      break;
    case 6:
      monthStr = "Jun";
      break;
    case 7:
      monthStr = "Jul";
      break;
    case 8:
      monthStr = "Aug";
      break;
    case 9:
      monthStr = "Sep";
      break;
    case 10:
      monthStr = "Oct";
      break;
    case 11:
      monthStr = "Nov";
      break;
    case 12:
      monthStr = "Dec";
      break;                                                
  }

  lcd.print(monthStr);

  // Print date on LCD.
  valStr[0] = msgData[2];
  valStr[1] = msgData[3];
  valStr[2] = 0;

  lcd.setCursor(4, 1);
  lcd.print(valStr);

  // Print hour on LCD.
  valStr[0] = msgData[4];
  valStr[1] = msgData[5];
  valStr[2] = 0;  

  lcd.setCursor(11, 1);
  lcd.print(valStr);

  // Print time seperator.
  lcd.print(':');

  //Print minutes on LCD.
  valStr[0] = msgData[6];
  valStr[1] = msgData[7];
  valStr[2] = 0; 

  lcd.print(valStr);
}

void setup() 
{
  cidState = CID_IDLE;
  cidMsg = CIDMSG_HEADER;
  
  // Initialize serial port for debugging (baud rate: 115200, parameters: 8N1). 
#ifdef DEBUG_LOGS
  Serial.begin(115200);
#endif

  // Configure I/O ports.

  // Setup call alert LED pin.
  pinMode(LCD_CALL_ALERT_LED, OUTPUT);
  digitalWrite(LCD_CALL_ALERT_LED, LOW);

  // Setup LCD backlight controller pin.
  pinMode(LCD_BACKLED, OUTPUT);
  digitalWrite(LCD_BACKLED, LOW);
  
  // Setup line ringer status input pin.
  pinMode(RING_DET_PIN, INPUT);
  
  // Setup caller ID decoder power control pin.
  pinMode(CLI_PWDN_PIN, OUTPUT);
  
  // Power down the callerID at the system startup.
  digitalWrite(CLI_PWDN_PIN, HIGH);

  // Setup callerID pin modes.
  pinMode(CALLER_ID_RX, INPUT);
  pinMode(CaLLER_ID_TX, OUTPUT);

  // Initialize callerID interface with baud rate of 1200.
  callerID.begin(1200);

  // Initialize 1602 LCD.
  lcd.begin(16, 2);

  // Setup display/clear button.
  pinMode(A0, INPUT_PULLUP);
  buttonState = digitalRead(A0);

  // Reset virtual watchdog timer/counter.
  watchdogDelay = micros();
  watchdogSec = 0;

  // Reset recall flags.
  recall = 0;
  recallTimeout = micros();
}

void loop() 
{
  // Check virtual watchdog timer to detect and handle communication/data losses.
  if((cidState == CID_SYNC) || (cidState == CID_PACKET) || (cidState == CID_MESSAGE))
  {
    // Watchdog expire-counter increments on every one second.
    if((micros() - watchdogDelay) >= 1000000)
    {
      watchdogSec++;
    }

    if(watchdogSec > 10)
    {
      // Watchdog timer is expired (with 10 sec window), lets reset the state machine.
      watchdogSec = 0;
      watchdogDelay = micros();

      // Switch state machine to idle state.
      cidState = CID_IDLE;
      cidMsg = CIDMSG_HEADER;
      tempCount = 0;
      lineIdleCounter = 0;
      msgCurrPos = 0;

      // Reset LCD screen.
      digitalWrite(LCD_BACKLED, LOW);
      lcd.clear();

      return;
    }
  }
    
  // Waiting for ring signal.
  if((cidState == CID_IDLE) && digitalRead(RING_DET_PIN))
  {
    // Activate callerID decoder and waiting for the data.
    pinMode(CLI_PWDN_PIN, LOW);
    tempCount = 0;

    // Reset virtual watchdog timer/counter.
    watchdogDelay = micros();
    watchdogSec = 0;
    
    cidState = CID_SYNC;
    
#ifdef DEBUG_LOGS
    Serial.write("INCOMING\n");
#endif

  }  
  else if(cidState == CID_END)
  {
    // Caller ID decoding is finished, lets wait until the ringing is finished.  
    if((micros() - delayStart) >= 100000)
    {
      delayStart = micros();
      lineIdleCounter++;

      // Reset virtual watchdog timer/counter.
      watchdogDelay = micros();
      watchdogSec = 0;

      if(digitalRead(RING_DET_PIN))
      {
        // Ring signal/session is still active.
        lineIdleCounter = 0;
      }
    }

    // Wait for 8 seconds to start new line sensing session.
    if(lineIdleCounter > 80)
    {
      // Clear and shutdown the LCD.
      digitalWrite(LCD_BACKLED, LOW);      
      lcd.clear();
      
      // Switch system to the idle state.
      lineIdleCounter = 0;
      cidState = CID_IDLE;
    }
  }
  else if((cidState == CID_SYNC) && (callerID.available()))
  {
    // Process sync data received from callerID and waiting for packet ID.
    cidData = callerID.read();

    // Reset virtual watchdog timer/counter.
    watchdogDelay = micros();
    watchdogSec = 0;

    // Checking for MDMF packet header.
    if((tempCount >= 25) && (cidData == MDMF_HEADER))
    {
      // MDMF packet header is detected.
#ifdef DEBUG_LOGS      
      Serial.write("MDMF PACKET\n");
#endif      

      // Activate LCD to display incoming caller ID data.
      lcd.clear();
      digitalWrite(LCD_BACKLED, HIGH);
     
      cidState = CID_PACKET;

      // Reset virtual watchdog timer/counter.
      watchdogDelay = micros();
      watchdogSec = 0;
    }
    
    if(cidData == 0x55)
    {
      // Sync byte detected, increment the sync data counter.
      tempCount++;                      
    }           
  }  
  else if((cidState == CID_PACKET) && (callerID.available()))
  {
    // Process MDMF packet data and extract packet length.
    packetLen = callerID.read();
    currLen = 0;
    cidState = CID_MESSAGE;
    cidMsg = CIDMSG_HEADER;

    // Reset virtual watchdog timer/counter.
    watchdogDelay = micros();
    watchdogSec = 0;
  }  
  else if((cidState == CID_MESSAGE) && (callerID.available()))
  {
    // Reached end of packet and waiting to process next caller Id packet.
    if(currLen >= packetLen)
    {
      
#ifdef DEBUG_LOGS     
      Serial.write("MDMF PACKET END\n");
#endif      
      
      cidState = CID_END;
      cidMsg = CIDMSG_HEADER;  

      // Initialize idle state monitoring variables.
      lineIdleCounter = 0;      
      delayStart = micros();

      // Reset virtual watchdog timer/counter.
      watchdogDelay = micros();
      watchdogSec = 0;

      // Read checksum byte.
      callerID.read();

      // Activate missed call alert.
      digitalWrite(LCD_CALL_ALERT_LED, HIGH);

      // Save call information into EEPROM.
      saveCallInfoToEEPROM();

      // Shutdown caller id decoder.
      pinMode(CLI_PWDN_PIN, HIGH);    
      return;
    }
    
    // Process messages in MDMF packet.
    if(cidMsg == CIDMSG_HEADER)
    {
      // Extract message type.
#ifdef DEBUG_LOGS      
      Serial.write(" MSG HEADER: ");                 
#endif      
      
      msgType = callerID.read();   

#ifdef DEBUG_LOGS      
      Serial.print(msgType, HEX);
      Serial.write("\n");
#endif      
      
      cidMsg = CIDMSG_LEN; 

      // Reset virtual watchdog timer/counter.
      watchdogDelay = micros();
      watchdogSec = 0;
      
      currLen++;  
    }
    else if(cidMsg == CIDMSG_LEN)
    {
      // Extract message length.
#ifdef DEBUG_LOGS       
      Serial.write(" MSG LEN: ");
#endif      
      
      msgLen = callerID.read();
      
#ifdef DEBUG_LOGS      
      Serial.print(msgLen, HEX);
      Serial.write("\n");
#endif       
      
      cidMsg = CIDMSG_DATA;

      // Reset virtual watchdog timer/counter.
      watchdogDelay = micros();
      watchdogSec = 0;
      
      msgCurrPos = 0;
      currLen++;
    }
    else if(cidMsg == CIDMSG_DATA)
    {
      // Extract message data up to 15 characters.
      if(msgCurrPos < 15)
      {
        msgData[msgCurrPos] = callerID.read();
        msgData[msgCurrPos + 1] = 0;
      }
      else
      {
        callerID.read();
      }
      
      msgCurrPos++;
      currLen++;

      // Checking for end of message.
      if(msgCurrPos >= msgLen)
      {
        // End of message data.

#ifdef DEBUG_LOGS         
        Serial.write(" MSG END\n");
        Serial.print(msgData);
        Serial.write("\n");
#endif        

        // Display decoded caller id message on LCD.
        switch(msgType)
        {
          case MDMF_PARAM_TIME: 
            // Print date/time information received from the exchange.                       
            printCallerIDTime();
            programLastCallTime();
            break;
          case MDMF_PARAM_CID:
            // Print calling line identification string (phone number).
            lcd.setCursor(0, 0);
            lcd.print(msgData);
            programLastNumber();
            break;
          case MDMF_PARAM_NAME:
            // TODO: Print name data received from the exchange.
            break;
        }

        // Process next message.
        cidMsg = CIDMSG_HEADER;

        // Reset virtual watchdog timer/counter.
        watchdogDelay = micros();
        watchdogSec = 0;
      }      
    }        
  }
  else if(cidState == CID_IDLE)
  {
    // System and phone line are on idle state.
    // Handle button press event.
    if((buttonState == 0) && (digitalRead(A0)))
    {
      // Display last call information on LCD and reset the missed call indicator.

      // Clear missed call alert indicator.
      digitalWrite(LCD_CALL_ALERT_LED, LOW);
      
      // Get last phone number from the EEPROM.
      for(tempInfoPos = 0; tempInfoPos < 16; tempInfoPos++)
      {
        msgData[tempInfoPos] = EEPROM.read(EEPROM_NUM_OFFSET + tempInfoPos);
      }

      lcd.clear();
      digitalWrite(LCD_BACKLED, HIGH);

      // Update recall flag.
      recall = 1;
      recallTimeout = micros();
      
      // Check for valid phone number.
      if((msgData[0] == 0) || (msgData[0] == 0xFF))
      {
        // Phone data in EEPROM is invalid or not programmed.        
        lcd.write("No information");
      }
      else
      {
        // Print phone number on LCD.
        lcd.setCursor(0, 0);
        lcd.print(msgData);    

        // Get date and time related to last call (from the EEPROM).
        for(tempInfoPos = 0; tempInfoPos < 9; tempInfoPos++)
        {
          msgData[tempInfoPos] = EEPROM.read(EEPROM_TIME_OFFSET + tempInfoPos);
        }

        // Print date/time on LCD.
        printCallerIDTime();        
      }
    }

    // Check timeout to shutdown the LCD which is activated by the user.
    if((recall) && ((micros() - recallTimeout) >= 8000000))
    {
      // Recall timeout occured to shutdown the LCD.
      recall = 0;
      digitalWrite(LCD_BACKLED, LOW);
      lcd.clear();
    }

    buttonState = digitalRead(A0);
  }
  
}
