////////////////////////////////////////////////////////////////////////////////
// TinyBasic Plus Xts
////////////////////////////////////////////////////////////////////////////////
//
// Modified by: Franck GALLIAT -- Xtase <fgalliat@gmail.com> on Nov 2016 -> va.15
//
// Authors: Mike Field <hamster@snap.net.nz>
//      Scott Lawrence <yorgle@gmail.com>
//         Brian O'Dell <megamemnon@megamemnon.com>

// IF testing with Visual C, this needs to be the first thing in the file.
//#include "stdafx.h"

// Teensy ++2 mode
#define teen2 1

// ===== String for http page serving (for ex.) ====
// redirects output to String buffer
bool bufferedOutput = false;
//String outputBuffer;

#define OUT_BUFF_LEN 1024
//char* outputBuffer = NULL;
char outputBuffer[OUT_BUFF_LEN];
int outputBufferCursor = 0;

void cleanOutBuff() { 
  //memset(&outputBuffer, 0, OUT_BUFF_LEN); 
  for(int i=0; i < OUT_BUFF_LEN; i++) { outputBuffer[i]=0x00; }
  outputBufferCursor = 0; 
}

// =================================================

#define WIFI_SUPPORT 1
#define LCD_SUPPORT 1

#define ALT_SER_PORT 1
//#undef ALT_SER_PORT

// '#1' representing ALT_SERIAL
#define ALT_SER_FILE 1
//#undef ALT_SER_FILE


#if defined(teen2)  
  #define __AVR_ATmega32U4__ 1
#endif

void chain(const char* fname);

#if defined(__AVR_ATmega32U4__)
  #define ENABLE_FILEIO 1
#else
  #undef ENABLE_FILEIO
#endif  

#define kVersion "va.15"

static unsigned char *program_end;
static unsigned char *variables_begin;

// in bytes
int getFreeMem() {
  return (variables_begin-program_end);
}

// ******************************************************

#if defined(LCD_SUPPORT)  
  // on Teensy ++2 -> SDA #1 SCL#0
  #include <Wire.h>
  #include <Adafruit_GFX.h>

  // local version
  #include "Adafruit_SSD1306.h"

  // broken 64 defined as 32 pixels height
  // it auto doubles the lines

  #define OLED_RESET 4
  Adafruit_SSD1306 display(OLED_RESET);


  void printmsgNoNLLCD(const unsigned char *msg) {
    while( pgm_read_byte( msg ) != 0 ) {
      display.print( (char)pgm_read_byte( msg++ ) );
    };
  }

  void writeOnLCD(const char* title, const char* str, const char* str2 = NULL) {
    display.clearDisplay();
    display.setCursor(0,0);
    if ( title == NULL ) {
      display.println(" -=| Xts uBasic  |=- ");
    } else {
      display.println(title);
    }
    display.println(str);
    if ( str2 != NULL ) {
      display.println(str2);  
    }
    display.display();
  }

#else

  void writeOnLCD(const char* title, const char* str, const char* str2 = NULL) {
    // do nothing 
  }

#endif

// ******************************************************
  static const unsigned char initmsg[]          PROGMEM = "TinyBasic Plus " kVersion;

  #if defined(teen2)
    static const unsigned char cpuMsg[]          PROGMEM = "on a Teensy ++2 MCU";
  #elif defined(__AVR_ATmega32U4__)
    static const unsigned char cpuMsg[]          PROGMEM = "on a 32u4 MCU";
  #elif defined(__AVR_ATmega328P__)
    static const unsigned char cpuMsg[]          PROGMEM = "on a 328P MCU";
  #else  
    static const unsigned char cpuMsg[]          PROGMEM = "on a ??? MCU";
  #endif

  #if defined (ENABLE_FILEIO)
    static const unsigned char ioMsg[]          PROGMEM = "SDIO enabled";
    void cmd_Files(void);
  #else  
    static const unsigned char ioMsg[]          PROGMEM = "SDIO disabled";
  #endif
// ******************************************************

bool WIFI_OK = false;
#if defined(WIFI_SUPPORT)
  #define WIFI_BUFF_SIZE 128
  char wifiBuff[WIFI_BUFF_SIZE];

  void emptyWifiBuff() { for(int i=0; i < WIFI_BUFF_SIZE; i++) { wifiBuff[i] = 0x00; } }
    
  #if defined(teen2)  
    // on Teensy ++2 -> RX #2 TX#3
    #define HWSERIAL Serial1
  #endif

  #define WIFI_CMD_DEF_TIMEOUT 200

  // cmd w/o cr/lf
  void sendPassiveCmdToWifi(const char* cmd, bool outToScreen = true, bool outToSerial = true, bool checkWifiFlag = false) {
    HWSERIAL.print( cmd );
    HWSERIAL.print( "\r\n" );

    // keep it 'cause it's redefined in wait text
    HWSERIAL.setTimeout( WIFI_CMD_DEF_TIMEOUT );
    
    long t0, t1;
    t0 = millis();
    emptyWifiBuff();
    while( HWSERIAL.available() == 0 ) { t1 = millis(); if (t1-t0 > 1000) { break; } delay(1); }
    int readed = 0;
    do {
      readed = HWSERIAL.readBytesUntil(0x00, wifiBuff, WIFI_BUFF_SIZE);
      if (checkWifiFlag) { WIFI_OK = readed > 0; }
      if ( readed <= 0 ) { break; }
      if (outToScreen)   { writeOnLCD(" -= ESP 8266 says =- ", (const char*)wifiBuff); }
      if (outToSerial)   { Serial.println(wifiBuff); }
      emptyWifiBuff();
    } while (true);
  }

  void sendTextToClient(int client, const char* str, bool toScr = false, bool toSer = false) {
    char* cmd = (char*)malloc( 11 + 2 + 1 + 4 );
    sprintf(cmd, "AT+CIPSEND=%d,%d", client, strlen(str) );
    sendPassiveCmdToWifi((const char*)cmd, toScr, toSer); // 0 chanel -- 11 nb of bytes sent (max 2048)
    HWSERIAL.print( str );

    // just empty the buffer
    emptyWifiBuff();
    do {
      int readed = HWSERIAL.readBytesUntil(0x00, wifiBuff, WIFI_BUFF_SIZE);
      if ( readed <= 0 ) { break; }
      emptyWifiBuff();
    } while (true);
  }

//#define WIFI_SERV_MODE_TELNET 1
#undef WIFI_SERV_MODE_TELNET

  void wifiReset() {
    sendPassiveCmdToWifi("AT+RST", true, true); // reset module
    delay(500);
  }


  void wifiTestsFunc() {
    //sendPassiveCmdToWifi("AT+RST"); // reset module
    //delay(500);

    bool defScr=false, defSer=false;
    
    sendPassiveCmdToWifi("ATE0", defScr, defSer); // echo off -- ATE1 for echo on
    //sendPassiveCmdToWifi("AT+CIPAP?", true, true); // display IP

#ifdef WIFI_SERV_MODE_TELNET
    openTelnetServerAndWait(defScr,defSer);
#else
    openWebServerAndWait(defScr,defSer);
#endif

    // close conn.
    sendPassiveCmdToWifi("AT+CIPCLOSE=0", defScr, defSer); // 0 chanel
    // close server
    sendPassiveCmdToWifi("AT+CIPSERVER=0", defScr, defSer); // close server
  }

void waitFoClientConn(bool verbose = true) {
    if ( verbose ) {
      writeOnLCD(" -= ESP 8266 waits =- ", "Waiting conn.");
      Serial.println("Waiting conn.");
    }
    emptyWifiBuff();
    while( HWSERIAL.available() == 0 ) { delay(1); }
    int readed = 0;
    do {
      readed = HWSERIAL.readBytesUntil(0x00, wifiBuff, WIFI_BUFF_SIZE);
      if ( readed <= 0 ) { break; }
      //writeOnLCD(" -= ESP 8266 waits =- ", (const char*)wifiBuff);
      //Serial.println(wifiBuff);
      emptyWifiBuff();
    } while (true);
    // ===== has received something =====
}

const char* header =  "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n"; 

#define contentSize 1024
char content[contentSize]; // the web page content

void openWebServerAndWait(bool defScr,bool defSer) {
  // web server mode
  
  // open server
    sendPassiveCmdToWifi("AT+CIPMUX=1", defScr, defSer); // 1 -> allow multiple connections
    sendPassiveCmdToWifi("AT+CIPSERVER=1,80", defScr, defSer); // 1 server ON -- 23 TCP port
    //sendPassiveCmdToWifi("AT+CIPSTO=180", defScr, defSer); // server timeout in sec. -- 180sec is default

    // wait for a connection
    waitFoClientConn();

    writeOnLCD(" -= ESP 8266 page =- ", "Preparing page");
    Serial.println("Preparing page");

    for(int i=0; i < contentSize; i++) { content[i] = 0x00; }
    
    
    strcat( content, "<html><head><title>ESP8266 uXtsBasic</title></head>\n");
    strcat( content, "<body>\n");
    strcat( content, "<h1>Hello World from my Teensy ++2</h1>\n");
    strcat( content, "<table border=1><tr><td valign=top><pre>");

    char b[50];
    char myChar;
    int slen = strlen_P((const char*)initmsg);
    for (int k = 0; k < slen; k++) { myChar =  pgm_read_byte_near(initmsg + k); b[k] = myChar; }
    b[slen] = 0x0A; // as in a <pre>
    b[slen+1] = 0x00;
    strcat( content, b );

    slen = strlen_P((const char*)cpuMsg);
    for (int k = 0; k < slen; k++) { myChar =  pgm_read_byte_near(cpuMsg + k); b[k] = myChar; }
    b[slen] = 0x0A; // as in a <pre>
    b[slen+1] = 0x00;
    strcat( content, b );

    slen = strlen_P((const char*)ioMsg);
    for (int k = 0; k < slen; k++) { myChar =  pgm_read_byte_near(ioMsg + k); b[k] = myChar; }
    b[slen] = 0x0A; // as in a <pre>
    b[slen+1] = 0x00;
    strcat( content, b );

    strcat( content, "WIFI enabled\n" );

    sprintf( b, "%d bytes free\n", getFreeMem() );
    strcat( content, b );

    strcat( content, "</pre></td><td valign=top>\n" );    
    strcat( content, "<pre>" );

    sprintf( b, "Time: %d sec\n", (millis()/1000) );
    strcat( content, b );

    strcat( content, "</pre>\n");
    strcat( content, "</td></tr></table>\n");
    // ==========================================

    const char* openCode = "<hr/><pre>";
    const char* closeCode = "</pre><hr/>";

    cleanOutBuff();
    bufferedOutput = true;
    chain("web.bas");
    bufferedOutput = false;
    
    // done lower
    //cleanOutBuff();
    
    // ==========================================
    
    const char* closeHtml = "</body></html>\n";

    // can try to not set content-length if > 2048
    //header += "Content-Length: ";
    //header += (int)(content.length());
    ////header += "\r\n\r\n";

    int headerlen = strlen( header ) + 4; // +4 for \r\n\r\n
    //int contentLen = strlen( content );
    int contentLen = strlen(content)+strlen(openCode)+strlen(outputBuffer)+strlen(closeCode)+strlen(closeHtml);

    int ch_id = 0;
    HWSERIAL.print("AT+CIPSEND=");
    HWSERIAL.print(ch_id);
    HWSERIAL.print(",");
    HWSERIAL.println(headerlen+contentLen);

    if (HWSERIAL.find(">")) {
      HWSERIAL.print(header);
      HWSERIAL.print("\r\n\r\n");
      
      HWSERIAL.print(content);
      HWSERIAL.print(openCode);
      HWSERIAL.print(outputBuffer);
      HWSERIAL.print(closeCode);

      HWSERIAL.print(closeHtml);
      
      delay(10);     
    } 

    cleanOutBuff();

    //free(content);
    //content = NULL;

    writeOnLCD(" -= ESP 8266 sent =- ", "Served page");
    Serial.println("Served page on TCP:80");
}


void openTelnetServerAndWait(bool defScr,bool defSer) {
    int readed = 0;
    
    // open server
    sendPassiveCmdToWifi("AT+CIPMUX=1", defScr, defSer); // 1 -> allow multiple connections
    sendPassiveCmdToWifi("AT+CIPSERVER=1,23", defScr, defSer); // 1 server ON -- 23 TCP port
    sendPassiveCmdToWifi("AT+CIPSTO=180", defScr, defSer); // server timeout in sec.

    // wait for a connection
    waitFoClientConn();

    sendTextToClient( 0, "Hello from my ESP8266 on uXtsBasic\r\n$>" );
    // === wait for some text =========
    writeOnLCD(" -= ESP 8266 waits =- ", "Waiting text");
    Serial.println("Waiting text");
    emptyWifiBuff();
    HWSERIAL.setTimeout( 700 );
    while( HWSERIAL.available() == 0 ) { delay(1); }
    readed = 0;
//    +IPD,0,1:c
//    +IPD,0,1:o
// ...
//    +IPD,0,1:
    do {
      readed = HWSERIAL.readBytesUntil('/', wifiBuff, WIFI_BUFF_SIZE);
      if ( readed <= 0 ) { break; }
      writeOnLCD("  -= ESP 8266 rcv =- ", (const char*)wifiBuff);
      Serial.println(wifiBuff);
      emptyWifiBuff();
    } while (true);
    HWSERIAL.setTimeout( WIFI_CMD_DEF_TIMEOUT );
    // ===== has received something =====


    sendTextToClient( 0, "Bye from my ESP8266 on uXtsBasic\r\n" );
}


  
#endif


// ******************************************************


#if defined(__AVR_ATmega32U4__)
  #if defined(teen2)  
    #define XTS_SD_CS_PIN 20 
  #else
    #define XTS_SD_CS_PIN SS
  #endif
#else
  #define XTS_SD_CS_PIN 4
#endif

// _____________________________________

#ifdef ALT_SER_PORT
  // ADDED :
  //   KW_INIT, KW_INP, KW_OUT,
  //   KW_LLIST, KW_LPRINT,

  //#define ALT_SER_PORT_X07_MODE 1
  //#undef ALT_SER_PORT_X07_MODE
  
  #include "CustomSoftwareSerial_xtase.h"
  #ifdef ALT_SER_PORT_X07_MODE
   #define ALT_SER_PORT_SPEED 8000
   CustomSoftwareSerialXts altSerial(9,10, true);
  #else
   // can up du 19200 --> 115200 : garbage on port (from CustomSerial.h)
   #define ALT_SER_PORT_SPEED 9600
   #if defined(teen2)  
     CustomSoftwareSerialXts altSerial(26,25, false);
   #else
     CustomSoftwareSerialXts altSerial(9,10, false);
   #endif
  #endif

#endif



// _____________________________________

//#define XTS_CONSOLE_SPEED 115200
// @ 115200 it fails !!!! 9600 fails too @ some characters
// that was a memory lack issue
#define XTS_CONSOLE_SPEED 9600

// This enables LOAD, SAVE, FILES commands through the Arduino SD Library
// it adds 9k of usage as well.
// NOT ENOUGH RAM on UNO
//#define ENABLE_FILEIO 1










char eliminateCompileErrors = 1;  // fix to suppress arduino build errors

// hack to let makefiles work with this file unchanged
#ifdef FORCE_DESKTOP 
#undef ARDUINO
#else
#define ARDUINO 1
#endif


////////////////////////////////////////////////////////////////////////////////
// Feature option configuration...

// This enables LOAD, SAVE, FILES commands through the Arduino SD Library
// it adds 9k of usage as well.
//#define ENABLE_FILEIO 1
//#undef ENABLE_FILEIO

// this turns on "autorun".  if there's FileIO, and a file "autorun.bas",
// then it will load it and run it when starting up
//#define ENABLE_AUTORUN 1
#undef ENABLE_AUTORUN
// and this is the file that gets run
#define kAutorunFilename  "autorun.bas"

// this is the alternate autorun.  Autorun the program in the eeprom.
// it will load whatever is in the EEProm and run it
//#define ENABLE_EAUTORUN 1
#undef ENABLE_EAUTORUN

// this will enable the "TONE", "NOTONE" command using a piezo
// element on the specified pin.  Wire the red/positive/piezo to the kPiezoPin,
// and the black/negative/metal disc to ground.
// it adds 1.5k of usage as well.
//#define ENABLE_TONES 1
#undef ENABLE_TONES
#define kPiezoPin 5

// we can use the EEProm to store a program during powerdown.  This is 
// 1kbyte on the '328, and 512 bytes on the '168.  Enabling this here will
// allow for this funcitonality to work.  Note that this only works on AVR
// arduino.  Disable it for DUE/other devices.
//#define ENABLE_EEPROM 1
#undef ENABLE_EEPROM

// Sometimes, we connect with a slower device as the console.
// Set your console D0/D1 baud rate here (9600 baud default)
//#define kConsoleBaud 9600
#define kConsoleBaud XTS_CONSOLE_SPEED

////////////////////////////////////////////////////////////////////////////////
#ifdef ARDUINO
#ifndef RAMEND
// okay, this is a hack for now
// if we're in here, we're a DUE probably (ARM instead of AVR)

#define RAMEND 4096-1

// turn off EEProm
#undef ENABLE_EEPROM
#undef ENABLE_TONES

#else
// we're an AVR!

// we're moving our data strings into progmem
#include <avr/pgmspace.h>
#endif

// includes, and settings for Arduino-specific functionality
#ifdef ENABLE_EEPROM
#include <EEPROM.h>  /* NOTE: case sensitive */
int eepos = 0;
#endif


#ifdef ENABLE_FILEIO
#include <SD.h>
#include <SPI.h> /* needed as of 1.5 beta */

// Arduino-specific configuration
// set this to the card select for your SD shield
#define kSD_CS XTS_SD_CS_PIN

#define kSD_Fail  0
#define kSD_OK    1

File fp;
#endif

// set up our RAM buffer size for program and user input
// NOTE: This number will have to change if you include other libraries.
#ifdef ARDUINO
 #ifdef ENABLE_FILEIO
  #define kRamFileIO (1030) /* approximate */
 #else
  #define kRamFileIO (0)
 #endif
 
 #ifdef ENABLE_TONES
  #define kRamTones (40)
 #else
  #define kRamTones (0)
 #endif
#endif /* ARDUINO */

#if defined(LCD_SUPPORT)
  #define kRamLCD (940)
#else
  #define kRamLCD (0)
#endif

#if defined(WIFI_SUPPORT)
  #define kRamWIFI (WIFI_BUFF_SIZE + 1024 + 1024 + 512)
#else
  #define kRamWIFI (0)
#endif

// Cf issue #18
//#define kRamSize  (RAMEND - 1160 - kRamFileIO - kRamTones) 
#define kRamSize  (RAMEND - 1200 - kRamFileIO - kRamTones - kRamLCD - kRamWIFI) 


#ifndef ARDUINO
 // Not arduino setup
 #include <stdio.h>
 #include <stdlib.h>
 #undef ENABLE_TONES
 // size of our program ram
 // XTS FIX on a AT328 MEM is 2K & only 1K free
 #define kRamSize   4096 /* arbitrary */
 #ifdef ENABLE_FILEIO
  FILE * fp;
 #endif
#endif

#ifdef ENABLE_FILEIO
// functions defined elsehwere
void cmd_Files( void );
#endif

////////////////////

#ifndef boolean 
#define boolean int
#define true 1
#define false 0
#endif
#endif

#ifndef byte
typedef unsigned char byte;
#endif

// some catches for AVR based text string stuff...
#ifndef PROGMEM
 #define PROGMEM
#endif
#ifndef pgm_read_byte
 #define pgm_read_byte( A ) *(A)
#endif

////////////////////

#ifdef ENABLE_FILEIO
 unsigned char * filenameWord(void);
 static boolean sd_is_initialized = false;
#endif

boolean inhibitOutput = false;
static boolean runAfterLoad = false;
static boolean triggerRun = false;

// these will select, at runtime, where IO happens through for load/save
enum {
  kStreamSerial = 0,
  kStreamEEProm,
  kStreamFile
};
static unsigned char inStream = kStreamSerial;
static unsigned char outStream = kStreamSerial;


////////////////////////////////////////////////////////////////////////////////
// ASCII Characters
#define CR  '\r'
#define NL  '\n'
#define LF      0x0a
#define TAB '\t'
#define BELL  '\b'
#define SPACE   ' '
#define SQUOTE  '\''
#define DQUOTE  '\"'
#define CTRLC 0x03
#define CTRLH 0x08
#define CTRLS 0x13
#define CTRLX 0x18
// Xts
#define HASH '#'

typedef short unsigned LINENUM;
#ifdef ARDUINO
 #define ECHO_CHARS 1
#else
 #define ECHO_CHARS 0
#endif


static unsigned char program[kRamSize];
static const char *  sentinel = "HELLO";
static unsigned char *txtpos,*list_line, *tmptxtpos;
static unsigned char expression_error;
static unsigned char *tempsp;

/***********************************************************/
// Keyword table and constants - the last character has 0x80 added to it
const static unsigned char keywords[] PROGMEM = {
  'L','I','S','T'+0x80,
  'L','O','A','D'+0x80,
  'N','E','W'+0x80,
  'R','U','N'+0x80,
  'S','A','V','E'+0x80,
  'N','E','X','T'+0x80,
  'L','E','T'+0x80,
  'I','F'+0x80,
  'G','O','T','O'+0x80,
  'G','O','S','U','B'+0x80,
  'R','E','T','U','R','N'+0x80,
  'R','E','M'+0x80,
  'F','O','R'+0x80,
  'I','N','P','U','T'+0x80,
  'P','R','I','N','T'+0x80,
  'P','O','K','E'+0x80,
  'S','T','O','P'+0x80,
  'B','Y','E'+0x80,
  'F','I','L','E','S'+0x80,
  'M','E','M'+0x80,
  '?'+ 0x80,
  '\''+ 0x80,
  'A','W','R','I','T','E'+0x80,
  'D','W','R','I','T','E'+0x80,
  'D','E','L','A','Y'+0x80,
  'E','N','D'+0x80,
  'R','S','E','E','D'+0x80,
  'C','H','A','I','N'+0x80,
#ifdef ENABLE_TONES
  'T','O','N','E','W'+0x80,
  'T','O','N','E'+0x80,
  'N','O','T','O','N','E'+0x80,
#endif
#ifdef ARDUINO
 #ifdef ENABLE_EEPROM
  'E','C','H','A','I','N'+0x80,
  'E','L','I','S','T'+0x80,
  'E','L','O','A','D'+0x80,
  'E','F','O','R','M','A','T'+0x80,
  'E','S','A','V','E'+0x80,
 #endif
#endif

// Xtase ioFileDescriptor
#ifdef ALT_SER_FILE
  'I','N','I','T'+0x80,
  'I','N','P'+0x80, // ====> TO MOVE TO FUNCTIONS !!!!!
  'O','U','T'+0x80,
  'L','L','I','S','T'+0x80,
  'L','P','R','I','N','T'+0x80,
#endif

// Xtase Esp8266 Wifi routines
#ifdef WIFI_SUPPORT
  'W','T','E','S','T'+0x80,
  'W','R','E','S','E','T'+0x80, // reset the 8266
#endif


// Xtase extended functions
  'D','E','L','E','T','E'+0x80,
  'C','A','T'+0x80,
  
  0
};

// by moving the command list to an enum, we can easily remove sections 
// above and below simultaneously to selectively obliterate functionality.
enum {
  KW_LIST = 0,
  KW_LOAD, KW_NEW, KW_RUN, KW_SAVE,
  KW_NEXT, KW_LET, KW_IF,
  KW_GOTO, KW_GOSUB, KW_RETURN,
  KW_REM,
  KW_FOR,
  KW_INPUT, KW_PRINT,
  KW_POKE,
  KW_STOP, KW_BYE,
  KW_FILES,
  KW_MEM,
  KW_QMARK, KW_QUOTE,
  KW_AWRITE, KW_DWRITE,
  KW_DELAY,
  KW_END,
  KW_RSEED,
  KW_CHAIN,
#ifdef ENABLE_TONES
  KW_TONEW, KW_TONE, KW_NOTONE,
#endif
#ifdef ARDUINO
 #ifdef ENABLE_EEPROM
  KW_ECHAIN, KW_ELIST, KW_ELOAD, KW_EFORMAT, KW_ESAVE, 
 #endif
#endif

#ifdef ALT_SER_FILE
  KW_INIT, KW_INP, KW_OUT,
  KW_LLIST, KW_LPRINT,
#endif

#ifdef WIFI_SUPPORT
  KW_WTEST, 
  KW_WRESET,
#endif

// Xtase extended functions
  KW_DELETE,
  KW_CAT,

  KW_DEFAULT /* always the final one*/
};

struct stack_for_frame {
  char frame_type;
  char for_var;
  short int terminal;
  short int step;
  unsigned char *current_line;
  unsigned char *txtpos;
};

struct stack_gosub_frame {
  char frame_type;
  unsigned char *current_line;
  unsigned char *txtpos;
};

const static unsigned char func_tab[] PROGMEM = {
  'P','E','E','K'+0x80,
  'A','B','S'+0x80,
  'A','R','E','A','D'+0x80,
  'D','R','E','A','D'+0x80,
  'R','N','D'+0x80,
// Xtase extended functions -->
  'F','R','E','E'+0x80,
// Xtase extended functions <--
  0
};
#define FUNC_PEEK    0
#define FUNC_ABS     1
#define FUNC_AREAD   2
#define FUNC_DREAD   3
#define FUNC_RND     4
#define FUNC_FREE    5
#define FUNC_UNKNOWN 6

const static unsigned char to_tab[] PROGMEM = {
  'T','O'+0x80,
  0
};

const static unsigned char step_tab[] PROGMEM = {
  'S','T','E','P'+0x80,
  0
};

const static unsigned char relop_tab[] PROGMEM = {
  '>','='+0x80,
  '<','>'+0x80,
  '>'+0x80,
  '='+0x80,
  '<','='+0x80,
  '<'+0x80,
  '!','='+0x80,
  0
};

#define RELOP_GE    0
#define RELOP_NE    1
#define RELOP_GT    2
#define RELOP_EQ    3
#define RELOP_LE    4
#define RELOP_LT    5
#define RELOP_NE_BANG   6
#define RELOP_UNKNOWN 7

const static unsigned char highlow_tab[] PROGMEM = { 
  'H','I','G','H'+0x80,
  'H','I'+0x80,
  'L','O','W'+0x80,
  'L','O'+0x80,
  0
};
#define HIGHLOW_HIGH    1
#define HIGHLOW_UNKNOWN 4

#define STACK_SIZE (sizeof(struct stack_for_frame)*5)
#define VAR_SIZE sizeof(short int) // Size of variables in bytes

static unsigned char *stack_limit;
static unsigned char *program_start;

//declared upper...
//static unsigned char *program_end;
//static unsigned char *variables_begin;

static unsigned char *stack; // Software stack for things that should go on the CPU stack

static unsigned char *current_line;
static unsigned char *sp;
#define STACK_GOSUB_FLAG 'G'
#define STACK_FOR_FLAG 'F'
static unsigned char table_index;
static LINENUM linenum;

static const unsigned char okmsg[]            PROGMEM = "OK";
static const unsigned char whatmsg[]          PROGMEM = "What? ";
static const unsigned char howmsg[]           PROGMEM = "How?";
static const unsigned char sorrymsg[]         PROGMEM = "Sorry!";
//static const unsigned char initmsg[]          PROGMEM = "TinyBasic Plus " kVersion;

//  #if defined(teen2)
//    static const unsigned char cpuMsg[]          PROGMEM = "on a Teensy ++2 MCU";
//  #elif defined(__AVR_ATmega32U4__)
//    static const unsigned char cpuMsg[]          PROGMEM = "on a 32u4 MCU";
//  #elif defined(__AVR_ATmega328P__)
//    static const unsigned char cpuMsg[]          PROGMEM = "on a 328P MCU";
//  #else  
//    static const unsigned char cpuMsg[]          PROGMEM = "on a ??? MCU";
//  #endif
//
//  #if defined (ENABLE_FILEIO)
//    static const unsigned char ioMsg[]          PROGMEM = "SDIO enabled";
//  #else  
//    static const unsigned char ioMsg[]          PROGMEM = "SDIO disabled";
//  #endif
  
static const unsigned char memorymsg[]        PROGMEM = " bytes free.";
#ifdef ARDUINO
 #ifdef ENABLE_EEPROM
static const unsigned char eeprommsg[]        PROGMEM = " EEProm bytes total.";
static const unsigned char eepromamsg[]       PROGMEM = " EEProm bytes available.";
 #endif
#endif
static const unsigned char breakmsg[]         PROGMEM = "break!";
static const unsigned char unimplimentedmsg[] PROGMEM = "Unimplemented";
static const unsigned char backspacemsg[]     PROGMEM = "\b \b";
static const unsigned char indentmsg[]        PROGMEM = "    ";
static const unsigned char sderrormsg[]       PROGMEM = "SD card error.";
static const unsigned char sdfilemsg[]        PROGMEM = "SD file error.";
static const unsigned char dirextmsg[]        PROGMEM = "(dir)";
static const unsigned char slashmsg[]         PROGMEM = "/";
static const unsigned char spacemsg[]         PROGMEM = " ";

#ifdef ENABLE_FILEIO
  void chain(const char* fname) {
    doRun( KW_CHAIN, fname );
  }
#else
  void chain(const char* fname) {
    // nothing
  }
#endif


#if defined(LCD_SUPPORT)  
  void startupScreen() {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(" -=| Xts uBasic  |=- ");
    //printmsgNoNLLCD(initmsg); display.println("");
    printmsgNoNLLCD(cpuMsg); display.println("");
    printmsgNoNLLCD(ioMsg); display.println("");
    if ( WIFI_OK ) {
      display.println("WIFI enabled");
    } else {
      display.println("WIFI disabled");
    }
    display.display();
  }
#endif



static int inchar(void);
static void outchar(unsigned char c);
static void line_terminator(void);
static short int expression(void);
static unsigned char breakcheck(void);
/***************************************************************************/
static void ignore_blanks(void)
{
  while(*txtpos == SPACE || *txtpos == TAB)
    txtpos++;
}


/***************************************************************************/
static void scantable(const unsigned char *table)
{
  int i = 0;
  table_index = 0;
  while(1)
  {
    // Run out of table entries?
    if(pgm_read_byte( table ) == 0)
      return;

    // Do we match this character?
    if(txtpos[i] == pgm_read_byte( table ))
    {
      i++;
      table++;
    }
    else
    {
      // do we match the last character of keywork (with 0x80 added)? If so, return
      if(txtpos[i]+0x80 == pgm_read_byte( table ))
      {
        txtpos += i+1;  // Advance the pointer to following the keyword
        ignore_blanks();
        return;
      }

      // Forward to the end of this keyword
      while((pgm_read_byte( table ) & 0x80) == 0)
        table++;

      // Now move on to the first character of the next word, and reset the position index
      table++;
      table_index++;
      ignore_blanks();
      i = 0;
    }
  }
}

/***************************************************************************/
static void pushb(unsigned char b)
{
  sp--;
  *sp = b;
}

/***************************************************************************/
static unsigned char popb()
{
  unsigned char b;
  b = *sp;
  sp++;
  return b;
}

/***************************************************************************/
void printnum(int num)
{
  int digits = 0;

  if(num < 0)
  {
    num = -num;
    outchar('-');
  }
  do {
    pushb(num%10+'0');
    num = num/10;
    digits++;
  }
  while (num > 0);

  while(digits > 0)
  {
    outchar(popb());
    digits--;
  }
}

void printUnum(unsigned int num)
{
  int digits = 0;

  do {
    pushb(num%10+'0');
    num = num/10;
    digits++;
  }
  while (num > 0);

  while(digits > 0)
  {
    outchar(popb());
    digits--;
  }
}

/***************************************************************************/
static unsigned short testnum(void)
{
  unsigned short num = 0;
  ignore_blanks();

  while(*txtpos>= '0' && *txtpos <= '9' )
  {
    // Trap overflows
    if(num >= 0xFFFF/10)
    {
      num = 0xFFFF;
      break;
    }

    num = num *10 + *txtpos - '0';
    txtpos++;
  }
  return  num;
}

/***************************************************************************/
static unsigned char print_quoted_string(void)
{
  int i=0;
  unsigned char delim = *txtpos;
  if(delim != '"' && delim != '\'')
    return 0;
  txtpos++;

  // Check we have a closing delimiter
  while(txtpos[i] != delim)
  {
    if(txtpos[i] == NL)
      return 0;
    i++;
  }

  // Print the characters
  while(*txtpos != delim)
  {
    outchar(*txtpos);
    txtpos++;
  }
  txtpos++; // Skip over the last delimiter

  return 1;
}

// Xts Xts Xts Xts Xts Xts Xts Xts Xts Xts 
static void lline_terminator(void)
{
  outchar(NL);
  outchar(CR);
}

void lprintline()
{
  LINENUM line_num;

  line_num = *((LINENUM *)(list_line));
  list_line += sizeof(LINENUM) + sizeof(char);

  // Output the line */
  lprintnum(line_num);
  l_outchar(' ');
  while(*list_line != NL)
  {
    l_outchar(*list_line);
    list_line++;
  }
  list_line++;
  lline_terminator();
}

void lprintnum(int num)
{
  int digits = 0;

  if(num < 0)
  {
    num = -num;
    l_outchar('-');
  }
  do {
    pushb(num%10+'0');
    num = num/10;
    digits++;
  }
  while (num > 0);

  while(digits > 0)
  {
    l_outchar(popb());
    digits--;
  }
}

static unsigned char lprint_quoted_string(void)
{
  int i=0;
  unsigned char delim = *txtpos;
  if(delim != '"' && delim != '\'')
    return 0;
  txtpos++;

  // Check we have a closing delimiter
  while(txtpos[i] != delim)
  {
    if(txtpos[i] == NL)
      return 0;
    i++;
  }

  // Print the characters
  while(*txtpos != delim)
  {
    l_outchar(*txtpos);
    txtpos++;
  }
  txtpos++; // Skip over the last delimiter

  return 1;
}


void printXts(const unsigned char *msg) {
  int len = strlen( (const char*)msg );
  for(int i=0; i < len; i++) {
    outchar( msg[i] );
  }
  //outchar( 13 );
  //outchar( 10 );
  //while( pgm_read_byte( msg ) != 0 ) {
  //  outchar( pgm_read_byte( msg++ ) );
  //};
}

// Xts Xts Xts Xts Xts Xts Xts Xts Xts Xts 


/***************************************************************************/
void printmsgNoNL(const unsigned char *msg)
{
  while( pgm_read_byte( msg ) != 0 ) {
    outchar( pgm_read_byte( msg++ ) );
  };
}

/***************************************************************************/
void printmsg(const unsigned char *msg)
{
  printmsgNoNL(msg);
  line_terminator();
}

/***************************************************************************/
static void getln(char prompt)
{
  outchar(prompt);
  txtpos = program_end+sizeof(LINENUM);

  while(1)
  {
    char c = inchar();
    switch(c)
    {
    case NL:
      //break;
    case CR:
      line_terminator();
      // Terminate all strings with a NL
      txtpos[0] = NL;
      return;
    case CTRLH:
      if(txtpos == program_end)
        break;
      txtpos--;

      printmsg(backspacemsg);
      break;
    default:
      // We need to leave at least one space to allow us to shuffle the line into order
      if(txtpos == variables_begin-2)
        outchar(BELL);
      else
      {
        txtpos[0] = c;
        txtpos++;
        outchar(c);
      }
    }
  }
}

/***************************************************************************/
static unsigned char *findline(void)
{
  unsigned char *line = program_start;
  while(1)
  {
    if(line == program_end)
      return line;

    if(((LINENUM *)line)[0] >= linenum)
      return line;

    // Add the line length onto the current address, to get to the next line;
    line += line[sizeof(LINENUM)];
  }
}

/***************************************************************************/
static void toUppercaseBuffer(void)
{
  unsigned char *c = program_end+sizeof(LINENUM);
  unsigned char quote = 0;

  while(*c != NL)
  {
    // Are we in a quoted string?
    if(*c == quote)
      quote = 0;
    else if(*c == '"' || *c == '\'')
      quote = *c;
    else if(quote == 0 && *c >= 'a' && *c <= 'z')
      *c = *c + 'A' - 'a';
    c++;
  }
}

/***************************************************************************/
void printline()
{
  LINENUM line_num;

  line_num = *((LINENUM *)(list_line));
  list_line += sizeof(LINENUM) + sizeof(char);

  // Output the line */
  printnum(line_num);
  outchar(' ');
  while(*list_line != NL)
  {
    outchar(*list_line);
    list_line++;
  }
  list_line++;
  line_terminator();
}

/***************************************************************************/
static short int expr4(void)
{
  // fix provided by Jurg Wullschleger wullschleger@gmail.com
  // fixes whitespace and unary operations
  ignore_blanks();

  if( *txtpos == '-' ) {
    txtpos++;
    return -expr4();
  }
  // end fix

  if(*txtpos == '0')
  {
    txtpos++;
    return 0;
  }

  if(*txtpos >= '1' && *txtpos <= '9')
  {
    short int a = 0;
    do  {
      a = a*10 + *txtpos - '0';
      txtpos++;
    } 
    while(*txtpos >= '0' && *txtpos <= '9');
    return a;
  }

  // Is it a function or variable reference?
  if(txtpos[0] >= 'A' && txtpos[0] <= 'Z')
  {
    short int a;
    // Is it a variable reference (single alpha)
    if(txtpos[1] < 'A' || txtpos[1] > 'Z')
    {
      a = ((short int *)variables_begin)[*txtpos - 'A'];
      txtpos++;
      return a;
    }

    // Is it a function with a single parameter
    scantable(func_tab);
    if(table_index == FUNC_UNKNOWN)
      goto expr4_error;

    unsigned char f = table_index;

    if(*txtpos != '(')
      goto expr4_error;

    txtpos++;
// Xts modif if no parameters ======
    //a = expression();
    if(*txtpos == ')') {
      a = -1;
    } else {
      a = expression();
    }
// Xts modif if no parameters ======
    
    if(*txtpos != ')')
      goto expr4_error;
    txtpos++;
    switch(f)
    {
    case FUNC_PEEK:
      return program[a];
      
    case FUNC_ABS:
      if(a < 0) 
        return -a;
      return a;

#ifdef ARDUINO
    case FUNC_AREAD:
      pinMode( a, INPUT );
      return analogRead( a );                        
    case FUNC_DREAD:
      pinMode( a, INPUT );
      return digitalRead( a );
#endif

    case FUNC_RND:
#ifdef ARDUINO
      return( random( a ));
#else
      return( rand() % a );
#endif

// Xtase extended fcts =======
    case FUNC_FREE:
      return getFreeMem();
// Xtase extended fcts =======


    }
  }

  if(*txtpos == '(')
  {
    short int a;
    txtpos++;
    a = expression();
    if(*txtpos != ')')
      goto expr4_error;

    txtpos++;
    return a;
  }

expr4_error:
  expression_error = 1;
  return 0;

}

/***************************************************************************/
static short int expr3(void)
{
  short int a,b;

  a = expr4();

  ignore_blanks(); // fix for eg:  100 a = a + 1

  while(1)
  {
    if(*txtpos == '*')
    {
      txtpos++;
      b = expr4();
      a *= b;
    }
    else if(*txtpos == '/')
    {
      txtpos++;
      b = expr4();
      if(b != 0)
        a /= b;
      else
        expression_error = 1;
    }
    else
      return a;
  }
}

/***************************************************************************/
static short int expr2(void)
{
  short int a,b;

  if(*txtpos == '-' || *txtpos == '+')
    a = 0;
  else
    a = expr3();

  while(1)
  {
    if(*txtpos == '-')
    {
      txtpos++;
      b = expr3();
      a -= b;
    }
    else if(*txtpos == '+')
    {
      txtpos++;
      b = expr3();
      a += b;
    }
    else
      return a;
  }
}
/***************************************************************************/
static short int expression(void)
{
  short int a,b;

  a = expr2();

  // Check if we have an error
  if(expression_error)  return a;

  scantable(relop_tab);
  if(table_index == RELOP_UNKNOWN)
    return a;

  switch(table_index)
  {
  case RELOP_GE:
    b = expr2();
    if(a >= b) return 1;
    break;
  case RELOP_NE:
  case RELOP_NE_BANG:
    b = expr2();
    if(a != b) return 1;
    break;
  case RELOP_GT:
    b = expr2();
    if(a > b) return 1;
    break;
  case RELOP_EQ:
    b = expr2();
    if(a == b) return 1;
    break;
  case RELOP_LE:
    b = expr2();
    if(a <= b) return 1;
    break;
  case RELOP_LT:
    b = expr2();
    if(a < b) return 1;
    break;
  }
  return 0;
}

/***************************************************************************/
  unsigned char *start;
  unsigned char *newEnd;

void loop() {
  
#ifdef ARDUINO
 #ifdef ENABLE_TONES
  noTone( kPiezoPin );
 #endif
#endif

  program_start = program;
  program_end = program_start;
  sp = program+sizeof(program);  // Needed for printnum
  stack_limit = program+sizeof(program)-STACK_SIZE;
  variables_begin = stack_limit - 27*VAR_SIZE;

  // memory free
  printnum(variables_begin-program_end);
  printmsg(memorymsg);
#ifdef ARDUINO
 #ifdef ENABLE_EEPROM
  // eprom size
  printnum( E2END+1 );
  printmsg( eeprommsg );
 #endif /* ENABLE_EEPROM */
#endif /* ARDUINO */


  doRun(-1, NULL);
}

// really do the job
//int cmdReq = -1;
void doRun(int cmd, const char* arg) {
  unsigned char linelen;
  boolean isDigital;
  boolean alsoWait = false;
  int val;


if ( cmd == KW_CHAIN ) {
  if( SD.exists( arg )) {
    program_end = program_start;
    fp = SD.open( arg );
    inStream = kStreamFile;
    inhibitOutput = true;
    runAfterLoad = true;
  }
  goto warmstart;
}


warmstart:
  // this signifies that it is running in 'direct' mode.
  current_line = 0;
  sp = program+sizeof(program);
  printmsg(okmsg);

prompt:
  if( triggerRun ){
    triggerRun = false;
    current_line = program_start;
    goto execline;
  }

  getln( '>' );
  toUppercaseBuffer();

  txtpos = program_end+sizeof(unsigned short);

  // Find the end of the freshly entered line
  while(*txtpos != NL) {
    txtpos++;
  }

  // Move it to the end of program_memory
  {
    unsigned char *dest;
    dest = variables_begin-1;
    while(1)
    {
      *dest = *txtpos;
      if(txtpos == program_end+sizeof(unsigned short))
        break;
      dest--;
      txtpos--;
    }
    txtpos = dest;
  }

  // Now see if we have a line number
  linenum = testnum();
  ignore_blanks();
  if(linenum == 0)
    goto direct;

  if(linenum == 0xFFFF)
    goto qhow;

  // Find the length of what is left, including the (yet-to-be-populated) line header
  linelen = 0;
  while(txtpos[linelen] != NL) {
    linelen++;
  }
  linelen++; // Include the NL in the line length
  linelen += sizeof(unsigned short)+sizeof(char); // Add space for the line number and line length

  // Now we have the number, add the line header.
  txtpos -= 3;
  *((unsigned short *)txtpos) = linenum;
  txtpos[sizeof(LINENUM)] = linelen;


  // Merge it into the rest of the program
  start = findline();

  // If a line with that number exists, then remove it
  if(start != program_end && *((LINENUM *)start) == linenum) {
    unsigned char *dest, *from;
    unsigned tomove;

    from = start + start[sizeof(LINENUM)];
    dest = start;

    tomove = program_end - from;
    while( tomove > 0) {
      *dest = *from;
      from++;
      dest++;
      tomove--;
    } 
    program_end = dest;
  }

  if(txtpos[sizeof(LINENUM)+sizeof(char)] == NL) { // If the line has no txt, it was just a delete
    goto prompt;
  }



  // Make room for the new line, either all in one hit or lots of little shuffles
  while(linelen > 0) { 
    unsigned int tomove;
    unsigned char *from,*dest;
    unsigned int space_to_make;

    space_to_make = txtpos - program_end;

    if(space_to_make > linelen)
      space_to_make = linelen;
    newEnd = program_end+space_to_make;
    tomove = program_end - start;


    // Source and destination - as these areas may overlap we need to move bottom up
    from = program_end;
    dest = newEnd;
    while(tomove > 0) {
      from--;
      dest--;
      *dest = *from;
      tomove--;
    }

    // Copy over the bytes into the new space
    for(tomove = 0; tomove < space_to_make; tomove++) {
      *start = *txtpos;
      txtpos++;
      start++;
      linelen--;
    }
    program_end = newEnd;
  }

  
  
  goto prompt;

unimplemented:
  printmsg(unimplimentedmsg);
  goto prompt;

qhow: 
  printmsg(howmsg);
  goto prompt;

qwhat:  
  printmsgNoNL(whatmsg);
  if(current_line != NULL)
  {
    unsigned char tmp = *txtpos;
    if(*txtpos != NL)
      *txtpos = '^';
    list_line = current_line;
    printline();
    *txtpos = tmp;
  }
  line_terminator();
  goto prompt;

qsorry: 
  printmsg(sorrymsg);
  goto warmstart;

run_next_statement:
  while(*txtpos == ':')
    txtpos++;
  ignore_blanks();
  if(*txtpos == NL)
    goto execnextline;
  goto interperateAtTxtpos;

direct: 
  txtpos = program_end+sizeof(LINENUM);
  if(*txtpos == NL)
    goto prompt;

interperateAtTxtpos:
  if(breakcheck())
  {
    printmsg(breakmsg);
    goto warmstart;
  }

  scantable(keywords);

  switch(table_index)
  {
  case KW_DELAY:
    {
#ifdef ARDUINO
      expression_error = 0;
      val = expression();
      delay( val );
      goto execnextline;
#else
      goto unimplemented;
#endif
    }

  case KW_FILES:
    goto files;
  case KW_LIST:
    goto list;
  case KW_CHAIN:
    goto chain;
  case KW_LOAD:
    goto load;
  case KW_MEM:
    goto mem;
  case KW_NEW:
    if(txtpos[0] != NL)
      goto qwhat;
    writeOnLCD(NULL, "NEW ....");
    program_end = program_start;
    goto prompt;
  case KW_RUN:
    writeOnLCD(NULL, "Run ....");
    
    current_line = program_start;
    goto execline;
  case KW_SAVE:
    goto save;
  case KW_NEXT:
    goto next;
  case KW_LET:
    goto assignment;
  case KW_IF:
    short int val;
    expression_error = 0;
    val = expression();
    if(expression_error || *txtpos == NL)
      goto qhow;
    if(val != 0)
      goto interperateAtTxtpos;
    goto execnextline;

  case KW_GOTO:
    expression_error = 0;
    linenum = expression();
    if(expression_error || *txtpos != NL)
      goto qhow;
    current_line = findline();
    goto execline;

  case KW_GOSUB:
    goto gosub;
  case KW_RETURN:
    goto gosub_return; 
  case KW_REM:
  case KW_QUOTE:
    goto execnextline;  // Ignore line completely
  case KW_FOR:
    goto forloop; 
  case KW_INPUT:
    goto input; 
  case KW_PRINT:
  case KW_QMARK:
    goto print;
  case KW_POKE:
    goto poke;
  case KW_END:
  case KW_STOP:
    // This is the easy way to end - set the current line to the end of program attempt to run it
    if(txtpos[0] != NL)
      goto qwhat;
    current_line = program_end;
    goto execline;
  case KW_BYE:
    // Leave the basic interperater
    startupScreen();
    return;

  case KW_AWRITE:  // AWRITE <pin>, HIGH|LOW
    isDigital = false;
    goto awrite;
  case KW_DWRITE:  // DWRITE <pin>, HIGH|LOW
    isDigital = true;
    goto dwrite;

  case KW_RSEED:
    goto rseed;

#ifdef ENABLE_TONES
  case KW_TONEW:
    alsoWait = true;
  case KW_TONE:
    goto tonegen;
  case KW_NOTONE:
    goto tonestop;
#endif

#ifdef ARDUINO
 #ifdef ENABLE_EEPROM
  case KW_EFORMAT:
    goto eformat;
  case KW_ESAVE:
    goto esave;
  case KW_ELOAD:
    goto eload;
  case KW_ELIST:
    goto elist;
  case KW_ECHAIN:
    goto echain;
 #endif
#endif

#ifdef ALT_SER_FILE
  case KW_INIT:
    goto ioInit;
  case KW_OUT:
    goto ioOut;
  case KW_INP:
    goto ioInp;
  case KW_LLIST:
    goto ioList;
  case KW_LPRINT:
    goto ioPrint;
#endif

#ifdef WIFI_SUPPORT
  case KW_WTEST:
    goto wifiTest;
  case KW_WRESET:
    goto wifiReset;
#endif

// XTase extended functions
  case KW_DELETE:
    goto sdDelete;
  case KW_CAT:
    goto sdCat;

  case KW_DEFAULT:
    goto assignment;
  default:
    break;
  }

execnextline:
  if(current_line == NULL)    // Processing direct commands?
    goto prompt;
  current_line +=  current_line[sizeof(LINENUM)];

execline:
  if(current_line == program_end) { // Out of lines to run
    if ( cmd == KW_CHAIN ) {
      // quit context
      Serial.println("Finished chain...");
      return;
    }
    goto warmstart;
  }
  txtpos = current_line+sizeof(LINENUM)+sizeof(char);
  goto interperateAtTxtpos;

#ifdef ARDUINO
 #ifdef ENABLE_EEPROM
elist:
  {
    int i;
    for( i = 0 ; i < (E2END +1) ; i++ )
    {
      val = EEPROM.read( i );

      if( val == '\0' ) {
        goto execnextline;
      }

      if( ((val < ' ') || (val  > '~')) && (val != NL) && (val != CR))  {
        outchar( '?' );
      } 
      else {
        outchar( val );
      }
    }
  }
  goto execnextline;

eformat:
  {
    for( int i = 0 ; i < E2END ; i++ )
    {
      if( (i & 0x03f) == 0x20 ) outchar( '.' );
      EEPROM.write( i, 0 );
    }
    outchar( LF );
  }
  goto execnextline;

esave:
  {
    outStream = kStreamEEProm;
    eepos = 0;

    // copied from "List"
    list_line = findline();
    while(list_line != program_end) {
      printline();
    }
    outchar('\0');

    // go back to standard output, close the file
    outStream = kStreamSerial;
    
    goto warmstart;
  }
  
  
echain:
  runAfterLoad = true;

eload:
  // clear the program
  program_end = program_start;

  // load from a file into memory
  eepos = 0;
  inStream = kStreamEEProm;
  inhibitOutput = true;
  goto warmstart;
 #endif /* ENABLE_EEPROM */
#endif

input:
  {
    unsigned char var;
    int value;
    ignore_blanks();
    if(*txtpos < 'A' || *txtpos > 'Z')
      goto qwhat;
    var = *txtpos;
    txtpos++;
    ignore_blanks();
    if(*txtpos != NL && *txtpos != ':')
      goto qwhat;
inputagain:
    tmptxtpos = txtpos;
    getln( '?' );
    toUppercaseBuffer();
    txtpos = program_end+sizeof(unsigned short);
    ignore_blanks();
    expression_error = 0;
    value = expression();
    if(expression_error)
      goto inputagain;
    ((short int *)variables_begin)[var-'A'] = value;
    txtpos = tmptxtpos;

    goto run_next_statement;
  }

forloop:
  {
    unsigned char var;
    short int initial, step, terminal;
    ignore_blanks();
    if(*txtpos < 'A' || *txtpos > 'Z')
      goto qwhat;
    var = *txtpos;
    txtpos++;
    ignore_blanks();
    if(*txtpos != '=')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    expression_error = 0;
    initial = expression();
    if(expression_error)
      goto qwhat;

    scantable(to_tab);
    if(table_index != 0)
      goto qwhat;

    terminal = expression();
    if(expression_error)
      goto qwhat;

    scantable(step_tab);
    if(table_index == 0)
    {
      step = expression();
      if(expression_error)
        goto qwhat;
    }
    else
      step = 1;
    ignore_blanks();
    if(*txtpos != NL && *txtpos != ':')
      goto qwhat;


    if(!expression_error && *txtpos == NL)
    {
      struct stack_for_frame *f;
      if(sp + sizeof(struct stack_for_frame) < stack_limit)
        goto qsorry;

      sp -= sizeof(struct stack_for_frame);
      f = (struct stack_for_frame *)sp;
      ((short int *)variables_begin)[var-'A'] = initial;
      f->frame_type = STACK_FOR_FLAG;
      f->for_var = var;
      f->terminal = terminal;
      f->step     = step;
      f->txtpos   = txtpos;
      f->current_line = current_line;
      goto run_next_statement;
    }
  }
  goto qhow;

gosub:
  expression_error = 0;
  linenum = expression();
  if(!expression_error && *txtpos == NL)
  {
    struct stack_gosub_frame *f;
    if(sp + sizeof(struct stack_gosub_frame) < stack_limit)
      goto qsorry;

    sp -= sizeof(struct stack_gosub_frame);
    f = (struct stack_gosub_frame *)sp;
    f->frame_type = STACK_GOSUB_FLAG;
    f->txtpos = txtpos;
    f->current_line = current_line;
    current_line = findline();
    goto execline;
  }
  goto qhow;

next:
  // Fnd the variable name
  ignore_blanks();
  if(*txtpos < 'A' || *txtpos > 'Z')
    goto qhow;
  txtpos++;
  ignore_blanks();
  if(*txtpos != ':' && *txtpos != NL)
    goto qwhat;

gosub_return:
  // Now walk up the stack frames and find the frame we want, if present
  tempsp = sp;
  while(tempsp < program+sizeof(program)-1)
  {
    switch(tempsp[0])
    {
    case STACK_GOSUB_FLAG:
      if(table_index == KW_RETURN)
      {
        struct stack_gosub_frame *f = (struct stack_gosub_frame *)tempsp;
        current_line  = f->current_line;
        txtpos      = f->txtpos;
        sp += sizeof(struct stack_gosub_frame);
        goto run_next_statement;
      }
      // This is not the loop you are looking for... so Walk back up the stack
      tempsp += sizeof(struct stack_gosub_frame);
      break;
    case STACK_FOR_FLAG:
      // Flag, Var, Final, Step
      if(table_index == KW_NEXT)
      {
        struct stack_for_frame *f = (struct stack_for_frame *)tempsp;
        // Is the the variable we are looking for?
        if(txtpos[-1] == f->for_var)
        {
          short int *varaddr = ((short int *)variables_begin) + txtpos[-1] - 'A'; 
          *varaddr = *varaddr + f->step;
          // Use a different test depending on the sign of the step increment
          if((f->step > 0 && *varaddr <= f->terminal) || (f->step < 0 && *varaddr >= f->terminal))
          {
            // We have to loop so don't pop the stack
            txtpos = f->txtpos;
            current_line = f->current_line;
            goto run_next_statement;
          }
          // We've run to the end of the loop. drop out of the loop, popping the stack
          sp = tempsp + sizeof(struct stack_for_frame);
          goto run_next_statement;
        }
      }
      // This is not the loop you are looking for... so Walk back up the stack
      tempsp += sizeof(struct stack_for_frame);
      break;
    default:
      //printf("Stack is stuffed!\n");
      goto warmstart;
    }
  }
  // Didn't find the variable we've been looking for
  goto qhow;

assignment:
  {
    short int value;
    short int *var;

    if(*txtpos < 'A' || *txtpos > 'Z')
      goto qhow;
    var = (short int *)variables_begin + *txtpos - 'A';
    txtpos++;

    ignore_blanks();

    if (*txtpos != '=')
      goto qwhat;
    txtpos++;
    ignore_blanks();
    expression_error = 0;
    value = expression();
    if(expression_error)
      goto qwhat;
    // Check that we are at the end of the statement
    if(*txtpos != NL && *txtpos != ':')
      goto qwhat;
    *var = value;
  }
  goto run_next_statement;
poke:
  {
    short int value;
    unsigned char *address;

    // Work out where to put it
    expression_error = 0;
    value = expression();
    if(expression_error)
      goto qwhat;
    address = (unsigned char *)value;

    // check for a comma
    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();

    // Now get the value to assign
    expression_error = 0;
    value = expression();
    if(expression_error)
      goto qwhat;
    //printf("Poke %p value %i\n",address, (unsigned char)value);
    // Check that we are at the end of the statement
    if(*txtpos != NL && *txtpos != ':')
      goto qwhat;
  }
  goto run_next_statement;

list:
  linenum = testnum(); // Retuns 0 if no line found.

  // Should be EOL
  if(txtpos[0] != NL)
    goto qwhat;

  // Find the line
  list_line = findline();
  while(list_line != program_end) {
    printline();
  }

  goto warmstart;

print:
  // If we have an empty list then just put out a NL
  if(*txtpos == ':' )
  {
    line_terminator();
    txtpos++;
    goto run_next_statement;
  }
  if(*txtpos == NL)
  {
    goto execnextline;
  }

  while(1)
  {
    ignore_blanks();
    if(print_quoted_string())
    {
      ;
    }
    else if(*txtpos == '"' || *txtpos == '\'')
      goto qwhat;
    else
    {
      short int e;
      expression_error = 0;
      e = expression();
      if(expression_error)
        goto qwhat;
      printnum(e);
    }

    // At this point we have three options, a comma or a new line
    if(*txtpos == ',')
      txtpos++; // Skip the comma and move onto the next
    else if(txtpos[0] == ';' && (txtpos[1] == NL || txtpos[1] == ':'))
    {
      txtpos++; // This has to be the end of the print - no newline
      break;
    }
    else if(*txtpos == NL || *txtpos == ':')
    {
      line_terminator();  // The end of the print statement
      break;
    }
    else
      goto qwhat; 
  }
  goto run_next_statement;

mem:
  // memory free
  printnum(variables_begin-program_end);
  printmsg(memorymsg);
#ifdef ARDUINO
 #ifdef ENABLE_EEPROM
  {
    // eprom size
    printnum( E2END+1 );
    printmsg( eeprommsg );
    
    // figure out the memory usage;
    val = ' ';
    int i;   
    for( i=0 ; (i<(E2END+1)) && (val != '\0') ; i++ ) {
      val = EEPROM.read( i );    
    }
    printnum( (E2END +1) - (i-1) );
    
    printmsg( eepromamsg );
  }
 #endif /* ENABLE_EEPROM */
#endif /* ARDUINO */
  goto run_next_statement;


  /*************************************************/

#ifdef ARDUINO
awrite: // AWRITE <pin>,val
dwrite:
  {
    short int pinNo;
    short int value;
    unsigned char *txtposBak;

    // Get the pin number
    expression_error = 0;
    pinNo = expression();
    if(expression_error)
      goto qwhat;

    // check for a comma
    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();


    txtposBak = txtpos; 
    scantable(highlow_tab);
    if(table_index != HIGHLOW_UNKNOWN)
    {
      if( table_index <= HIGHLOW_HIGH ) {
        value = 1;
      } 
      else {
        value = 0;
      }
    } 
    else {

      // and the value (numerical)
      expression_error = 0;
      value = expression();
      if(expression_error)
        goto qwhat;
    }
    pinMode( pinNo, OUTPUT );
    if( isDigital ) {
      digitalWrite( pinNo, value );
    } 
    else {
      analogWrite( pinNo, value );
    }
  }
  goto run_next_statement;
#else
pinmode: // PINMODE <pin>, I/O
awrite: // AWRITE <pin>,val
dwrite:
  goto unimplemented;
#endif

#ifdef ALT_SER_FILE
ioInit: // INIT 8000 => as INIT, "COM:", 8000, "B"
    // Get the speed
    short serSpeed;
    expression_error = 0;
    serSpeed = expression();
    if(expression_error)
      goto qwhat;

    //                1234567890123456789
    writeOnLCD(NULL, "Set ALT port speed");

    #ifdef ALT_SER_PORT
      //altSerial.close();
      altSerial.begin( serSpeed );
      altSerial.listen();
    #endif
  goto run_next_statement;
  
ioInp: // A = INP() => as A=INP(#1)
    #ifdef ALT_SER_PORT
    #endif
  goto unimplemented;
  
ioOut: // OUT 100 => as OUT #1,100
    // Get the value to send
    short value;
    expression_error = 0;
    value = expression();
    if(expression_error)
      goto qwhat;

    #ifdef ALT_SER_PORT
      altSerial.write( value );
    #endif
  goto run_next_statement;

ioList: // LLIST as LIST #1
  linenum = testnum(); // Retuns 0 if no line found.

  // Should be EOL
  if(txtpos[0] != NL)
    goto qwhat;

  // Find the line
  list_line = findline();
  while(list_line != program_end) {
    lprintline();
  }
  //goto warmstart;
  goto run_next_statement;
  
ioPrint: // LPRINT as PRINT #1, ....
  // If we have an empty list then just put out a NL
  if(*txtpos == ':' )
  {
    lline_terminator();
    txtpos++;
    goto run_next_statement;
  }
  if(*txtpos == NL)
  {
    goto execnextline;
  }

  while(1)
  {
    ignore_blanks();
    if(lprint_quoted_string())
    {
      ;
    }
    else if(*txtpos == '"' || *txtpos == '\'')
      goto qwhat;
    else
    {
      short int e;
      expression_error = 0;
      e = expression();
      if(expression_error)
        goto qwhat;
      lprintnum(e);
    }

    // At this point we have three options, a comma or a new line
    if(*txtpos == ',')
      txtpos++; // Skip the comma and move onto the next
    else if(txtpos[0] == ';' && (txtpos[1] == NL || txtpos[1] == ':'))
    {
      txtpos++; // This has to be the end of the print - no newline
      break;
    }
    else if(*txtpos == NL || *txtpos == ':')
    {
      lline_terminator();  // The end of the print statement
      break;
    }
    else
      goto qwhat; 
  }
  goto run_next_statement;
#endif

  /*************************************************/
files:
  // display a listing of files on the device.
  // version 1: no support for subdirectories

#ifdef ENABLE_FILEIO
    //                1234567890123456789
    writeOnLCD(NULL, "List SD content");
    cmd_Files();

  //if ( cmd == KW_CHAIN ) {
  //  goto run_next_statement;
  //} else {
  //  // as same as Canon X-07 does : exit from prgm
  //  goto warmstart;
  //}
  goto run_next_statement;
#else
  goto unimplemented;
#endif // ENABLE_FILEIO

chain:
  //                1234567890123456789
  writeOnLCD(NULL, "CHAIN");
  runAfterLoad = true;

load:
  // clear the program
  program_end = program_start;

  // load from a file into memory
#ifdef ENABLE_FILEIO
  {
    unsigned char *filename;

    // Work out the filename
    expression_error = 0;
    filename = filenameWord();
    if(expression_error)
      goto qwhat;

    //                1234567890123456789
    writeOnLCD(NULL, "Load file from SD", (const char*)filename);

#ifdef ARDUINO
    // Arduino specific
    if( !SD.exists( (char *)filename )) {
      printmsg( sdfilemsg );
    } 
    else {

      fp = SD.open( (const char *)filename );
      inStream = kStreamFile;
      inhibitOutput = true;
    }
#else // ARDUINO
    // Desktop specific
#endif // ARDUINO
    // this will kickstart a series of events to read in from the file.

  }
  goto warmstart;
#else // ENABLE_FILEIO
  goto unimplemented;
#endif // ENABLE_FILEIO



save:
  // save from memory out to a file
#ifdef ENABLE_FILEIO
  {
    unsigned char *filename;

    // Work out the filename
    expression_error = 0;
    filename = filenameWord();
    if(expression_error)
      goto qwhat;

    //                1234567890123456789
    writeOnLCD(NULL, "Save file to SD", (const char*)filename);

#ifdef ARDUINO
    // remove the old file if it exists
    if( SD.exists( (char *)filename )) {
      SD.remove( (char *)filename );
    }

    // open the file, switch over to file output
    fp = SD.open( (const char *)filename, FILE_WRITE );
    outStream = kStreamFile;

    // copied from "List"
    list_line = findline();
    while(list_line != program_end)
      printline();

    // go back to standard output, close the file
    outStream = kStreamSerial;

    fp.close();
#else // ARDUINO
    // desktop
#endif // ARDUINO
    goto warmstart;
  }
#else // ENABLE_FILEIO
  goto unimplemented;
#endif // ENABLE_FILEIO

rseed:
  {
    short int value;

    //Get the pin number
    expression_error = 0;
    value = expression();
    if(expression_error)
      goto qwhat;

#ifdef ARDUINO
    randomSeed( value );
#else // ARDUINO
    srand( value );
#endif // ARDUINO
    goto run_next_statement;
  }

/*************************************************/
wifiTest:
  // temp OpCode for wifi tests
#ifdef WIFI_SUPPORT
    wifiTestsFunc();
  goto run_next_statement;
#else
  goto unimplemented;
#endif

wifiReset:
  // OpCode for later wifi module reset
#ifdef WIFI_SUPPORT
    wifiReset();
  goto run_next_statement;
#else
  goto unimplemented;
#endif


/*************************************************/

sdDelete:
 #ifdef ENABLE_FILEIO
    unsigned char *filename;
    // Work out the filename
    expression_error = 0;
    filename = filenameWord();
    if(expression_error)
      goto qwhat;

    //                1234567890123456789
    writeOnLCD(NULL, "DELETE from SD", (const char*)filename);

    // remove the file if it exists
    if( SD.exists( (char *)filename )) {
      SD.remove( (char *)filename );
    }
    goto run_next_statement;
 #else
  goto unimplemented;
 #endif

sdCat:
 #ifdef ENABLE_FILEIO
    //unsigned char *filename;
    // Work out the filename
    expression_error = 0;
    filename = filenameWord();
    if(expression_error)
      goto qwhat;

    //                1234567890123456789
    writeOnLCD(NULL, "READ from SD", (const char*)filename);

    // remove the file if it exists
    if( SD.exists( (char *)filename )) {
      fp = SD.open( (const char *)filename, FILE_READ );
      while( fp.available() > 0 ) {
        outchar( fp.read() );
      }
      fp.close();
    }
    goto run_next_statement;
 #else
  goto unimplemented;
 #endif
 
/*************************************************/

#ifdef ENABLE_TONES
tonestop:
  noTone( kPiezoPin );
  goto run_next_statement;

tonegen:
  {
    // TONE freq, duration
    // if either are 0, tones turned off
    short int freq;
    short int duration;

    //Get the frequency
    expression_error = 0;
    freq = expression();
    if(expression_error)
      goto qwhat;

    ignore_blanks();
    if (*txtpos != ',')
      goto qwhat;
    txtpos++;
    ignore_blanks();


    //Get the duration
    expression_error = 0;
    duration = expression();
    if(expression_error)
      goto qwhat;

    if( freq == 0 || duration == 0 )
      goto tonestop;

    tone( kPiezoPin, freq, duration );
    if( alsoWait ) {
      delay( duration );
      alsoWait = false;
    }
    goto run_next_statement;
  }
#endif /* ENABLE_TONES */
}

// returns 1 if the character is valid in a filename
static int isValidFnChar( char c )
{
  if( c >= '0' && c <= '9' ) return 1; // number
  if( c >= 'A' && c <= 'Z' ) return 1; // LETTER
  if( c >= 'a' && c <= 'z' ) return 1; // letter (for completeness)
  if( c == '_' ) return 1;
  if( c == '+' ) return 1;
  if( c == '.' ) return 1;
  if( c == '~' ) return 1;  // Window~1.txt

  return 0;
}

unsigned char * filenameWord(void)
{
  // SDL - I wasn't sure if this functionality existed above, so I figured i'd put it here
  unsigned char * ret = txtpos;
  expression_error = 0;

  // make sure there are no quotes or spaces, search for valid characters
  //while(*txtpos == SPACE || *txtpos == TAB || *txtpos == SQUOTE || *txtpos == DQUOTE ) txtpos++;
  while( !isValidFnChar( *txtpos )) txtpos++;
  ret = txtpos;

  if( *ret == '\0' ) {
    expression_error = 1;
    return ret;
  }

  // now, find the next nonfnchar
  txtpos++;
  while( isValidFnChar( *txtpos )) txtpos++;
  if( txtpos != ret ) *txtpos = '\0';

  // set the error code if we've got no string
  if( *ret == '\0' ) {
    expression_error = 1;
  }

  return ret;
}

/***************************************************************************/
static void line_terminator(void)
{
  if ( !bufferedOutput ) { outchar(NL); }
  outchar(CR);
}

/***********************************************************/


void setup() {

#ifdef WIFI_SUPPORT
  //outputBuffer = (char*)malloc( OUT_BUFF_LEN );
  cleanOutBuff();
#endif

  
#ifdef ARDUINO
  #if defined(ALT_SER_PORT)
    #ifdef ALT_SER_PORT_X07_MODE 
      altSerial.begin(ALT_SER_PORT_SPEED, CSERIAL_8N2);
    #else
      //altSerial.begin(ALT_SER_PORT_SPEED, CSERIAL_8N1);
      altSerial.begin(ALT_SER_PORT_SPEED);
    #endif
  #endif

  Serial.begin(kConsoleBaud); // opens serial port

#if defined(LCD_SUPPORT)
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // display firware splash screen
  //display.display();
  //delay(500);

  display.clearDisplay();
  // reset the font
  display.setFont();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0,0);
  display.println("Hello, world!");
  display.println("12345678901234567890");
  display.println("12345678901234567890");
  display.println("12345678901234567890");
  display.display();
  
  #ifdef WIFI_SUPPORT
    HWSERIAL.begin(115200);
    sendPassiveCmdToWifi("AT+GMR", true, false, true);
    delay(500);
  #endif

  startupScreen();
  
#endif




  while( !Serial ); // for Leonardo

  //Serial.println( sentinel );
  printmsg(initmsg);
  printmsg(cpuMsg);
  printmsg(ioMsg);

  #ifdef WIFI_SUPPORT
    if ( WIFI_OK ) {
      printXts((const unsigned char*)"WIFI enabled\n");
    } else {
      printXts((const unsigned char*)"WIFI disabled\n");
    }
  #endif
  
  
#ifdef ENABLE_FILEIO
  initSD();
  
#ifdef ENABLE_AUTORUN
  if( SD.exists( kAutorunFilename )) {
    program_end = program_start;
    fp = SD.open( kAutorunFilename );
    inStream = kStreamFile;
    inhibitOutput = true;
    runAfterLoad = true;
  }
#endif /* ENABLE_AUTORUN */

#endif /* ENABLE_FILEIO */

#ifdef ENABLE_EEPROM
#ifdef ENABLE_EAUTORUN
  // read the first byte of the eeprom. if it's a number, assume it's a program we can load
  int val = EEPROM.read(0);
  if( val >= '0' && val <= '9' ) {
    program_end = program_start;
    inStream = kStreamEEProm;
    eepos = 0;
    inhibitOutput = true;
    runAfterLoad = true;
  }
#endif /* ENABLE_EAUTORUN */
#endif /* ENABLE_EEPROM */

#endif /* ARDUINO */
}


/***********************************************************/
static unsigned char breakcheck(void)
{
#ifdef ARDUINO
  if(Serial.available())
    return Serial.read() == CTRLC;
  return 0;
#else
#ifdef __CONIO__
  if(kbhit())
    return getch() == CTRLC;
  else
#endif
    return 0;
#endif
}
/***********************************************************/
static int inchar()
{
  int v;
#ifdef ARDUINO
  
  switch( inStream ) {
  case( kStreamFile ):
#ifdef ENABLE_FILEIO
    v = fp.read();
    if( v == NL ) v=CR; // file translate
    if( !fp.available() ) {
      fp.close();
      goto inchar_loadfinish;
    }
    return v;    
#else
#endif
     break;
  case( kStreamEEProm ):
#ifdef ENABLE_EEPROM
#ifdef ARDUINO
    v = EEPROM.read( eepos++ );
    if( v == '\0' ) {
      goto inchar_loadfinish;
    }
    return v;
#endif
#else
    inStream = kStreamSerial;
    return NL;
#endif
     break;
  case( kStreamSerial ):
  default:
    while(1)
    {
      if(Serial.available())
        return Serial.read();
    }
  }
  
inchar_loadfinish:
  inStream = kStreamSerial;
  inhibitOutput = false;

  if( runAfterLoad ) {
//    // Xts Xts Xts Xts Xts Xts 
//    // for http execution
//    bufferedOutput = false;
//finishedLoad = true;
//    // Xts Xts Xts Xts Xts Xts 
    
    runAfterLoad = false;
    triggerRun = true;
  }
  return NL; // trigger a prompt.
  
#else
  // otherwise. desktop!
  int got = getchar();

  // translation for desktop systems
  if( got == LF ) got = CR;

  return got;
#endif
}

/***********************************************************/
// Xts Xts Xts Xts Xts Xts Xts
static void l_outchar(unsigned char c) {
  #ifdef ALT_SER_PORT
    altSerial.write(c);
  #endif
}
// Xts Xts Xts Xts Xts Xts Xts

static void outchar(unsigned char c)
{
  if( inhibitOutput ) return;

#ifdef ARDUINO
  #ifdef ENABLE_FILEIO
    if( outStream == kStreamFile ) {
      // output to a file
      fp.write( c );
    } 
    else
  #endif
  #ifdef ARDUINO
  #ifdef ENABLE_EEPROM
    if( outStream == kStreamEEProm ) {
      EEPROM.write( eepos++, c );
    }
    else 
  #endif /* ENABLE_EEPROM */
  #endif /* ARDUINO */

    if ( !bufferedOutput ) {
      Serial.write(c);
    } else {
      //outputBuffer += (char)c;
      outputBuffer[ outputBufferCursor++ ] = (char)c;
      if ( outputBufferCursor >= OUT_BUFF_LEN ) { outputBufferCursor=0; } // rewind
    }

#else
  putchar(c);
#endif
}

/***********************************************************/
/* SD Card helpers */

#if ARDUINO && ENABLE_FILEIO

static int initSD( void )
{
  // if the card is already initialized, we just go with it.
  // there is no support (yet?) for hot-swap of SD Cards. if you need to 
  // swap, pop the card, reset the arduino.)

  if( sd_is_initialized == true ) return kSD_OK;

  // due to the way the SD Library works, pin 10 always needs to be 
  // an output, even when your shield uses another line for CS
  pinMode(kSD_CS, OUTPUT); // change this to 53 on a mega

  if( !SD.begin( kSD_CS )) {
    // failed
    printmsg( sderrormsg );
    return kSD_Fail;
  }
  // success - quietly return 0
  sd_is_initialized = true;

  // and our file redirection flags
  outStream = kStreamSerial;
  inStream = kStreamSerial;
  inhibitOutput = false;

  return kSD_OK;
}
#endif

#if ENABLE_FILEIO

void cmd_Files( void )
{
  File dir = SD.open( "/" );
  // XTase :: here is the trap ----
  dir.rewindDirectory();
  // XTase :: here is the trap ----
  dir.seek(0);

  while( true ) {
    File entry = dir.openNextFile();
    if( !entry ) {
      entry.close();
      break;
    }

    // common header
    printmsgNoNL( indentmsg );
// Moa Moa Moa Moa
    //printmsgNoNL( (const unsigned char *)entry.name() );
    printXts( (const unsigned char *)entry.name() );
// Moa Moa Moa Moa
    
    if( entry.isDirectory() ) {
      printmsgNoNL( slashmsg );
    }

    if( entry.isDirectory() ) {
      // directory ending
      for( int i=strlen( entry.name()) ; i<16 ; i++ ) {
        printmsgNoNL( spacemsg );
      }
      printmsgNoNL( dirextmsg );
    }
    else {
      // file ending
      for( int i=strlen( entry.name()) ; i<17 ; i++ ) {
        printmsgNoNL( spacemsg );
      }
      printUnum( entry.size() );
    }
    line_terminator();
    entry.close();
  }
  dir.close();
}

#endif
