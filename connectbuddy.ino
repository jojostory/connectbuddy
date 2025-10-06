/* TMGC+C Tamagotchi Plus color Connect buddy v1.04 August 2017 by Mr.Blinky
 *  
 *  v1.00 first public release
 *  v1.01 added serial monitor suport
 *  v1.02 added RGB LED with common annode support
 *  v1.03 added P's support
 *  v1.04 added Arduino Pro Micro support
 *  
 *  Functionality
 *  -------------
 *  
 *  After powering up the RGB LED will turn green and waits for an IR
 *  connection attempt or a button press.
 *  
 *  When an IR connection is initiated on the TMGC+C the connect buddy 
 *  responds by sending the 1000 points data frame.
 *  
 *  When the button is pressed on the connect buddy the RGB LED turns blue 
 *  indicating play mode and waits for the button to be released. 
 *  However if the button is hold down longer then half a second, marry mode 
 *  becomes active and the RGB LED will alternately flash cyan and purple.
 *  to allow you to choose the gender of the next generation tamagotchi. 
 *  Releasing the button on cyan will give you a boy and releasing on purple
 *  will give you a girl.
 *  
 *  After releasing the button in any mode, data is send to the TMGC+C and 
 *  the RGB LED changes color as following:
 *  
 *  -Yellow when a data frame is send by the Arduino.
 *  -Blue when the Arduino is waiting to receive a data frame.
 *  -Cyan when the Arduino is receiving a data frame
 *  
 * Each time you use the button mode the connect buddy identifies it self as 
 * a different friend (1 out of 14) to help you fill your notebook with friends. 
 * 
 * Note:
 * When you try to marry a non adult tamagotchi. your tamagotchi will shake 
 * his/her head and the connect buddy will briefly flash red to indicate the 
 * mariage was unsuccessful.
 * You can still marry off your non adult tamagotchi by connecting a second time.
 *  
 * Hardware setup using Arduino UNO (R3)
 * -------------------------------------
 *   
 *   ______|
 *  /     A|____/\/\/\_____Digital pin 4 Active High
 * (       | (1K resistor)
 *  \_____C|_______________GND
 *   IR LED      
 *   
 *   Emits infrared light when pin is HIGH. Used to send
 *   data pulses to Tamagotchi.
 *     ____
 *    |    |
 *   _| OUT|_____Digital pin 2 Active Low
 *  / |    | 
 * (  | GND|_____GND
 *  \_|    |
 *    | VSS|_____VCC
 *    |____|
 * 38KHz IR RECEIVER (TSOP4838 or other generic one)
 * 
 * Receives infrared ligth pulses from the tamagotchi and decodes 
 * them to logic LOW level. When there are no pulses detected the 
 * output will remain HIGH
 * 
 *   __________
 * _|   ____   |___ digital pin 3 active low
 *  |  /    \  |
 *  | |      | |
 *__|  \____/  |___ GND
 *  |__________|
 *  PUSH BUTTON (6x6mm tactile push button)
 * 
 * used to send play and marry commands to the tamagotchi.
 * when the button is pressed it's output will be LOW. When the
 * button is released the connection is open and it is read HIGH 
 * thanks to the pull-up feature of the input pins.
 * 
 *   ______|
 *  /     R|____/\/\/\_____     Digital pin 9 Active High
 * |      C|____________________GND 
 * |      G|____/\/\/\_______   Digital pin 11 Active High
 *  \_____B|____/\/\/\_____     Digital pin 10 Active High
 *         | (1K resistors)
 *  RGB LED (5mm, diffused, 4 pin, common cathode)
 *  
 ******************************************************************  
 * Start of code 
 ******************************************************************/

//pins and ports
#define IRVO       2
#define IRLED      4

#define BUTTON     3

#define LED_G      11
#define LED_B      10
#define LED_R      9

//timeout thresholds
#define TH_TIMEOUT  3000
#define TH_STARTBIT 2000
#define TH_1BIT     1000
#define TH_0BIT      500

//button values
#define BM_PLAY       0
#define BM_MARRY_BOY  1
#define BM_MARRY_GIRL 2

// frame format:
#define FRAME_LEN  20
#define FD_VERSION 0x00
#define FD_TYPE    0x01
#define FD_UID_LO  0x02
#define FD_TAMAID  0x03
#define FD_UID_HI  0x04
#define FD_NAME    0x05
#define FD_NAME1   0x05
#define FD_NAME2   0x06
#define FD_NAME3   0x07
#define FD_NAME4   0x08
#define FD_NAME5   0x09
#define FD_HAPPY   0x0A
#define FD_DATA    0x0B
#define FD_CSUM    0x13

//frame types
#define FT_CONNECT 0x00
#define FT_PLAY    0x02
#define FT_MARRY   0x06
#define FT_POINTS  0x70
#define FT_CANCEL  0xFC

//tama data
#define MALE   0
#define FEMALE 1
#define BABY   1
#define CHILD  2
#define TEEN   3
#define ADULT  4
#define OLDIE  5

typedef struct {
  byte   gender;
  byte   stage;
  String name;
} tamaData;

#define MAX_TAMAS 31
const tamaData tamaDataTable[MAX_TAMAS] = {
  //  TamaID   Gender  Stage  Name 
  { /* 0x00 */ MALE  , BABY , "KINOTCHI"               },
  { /* 0x01 */ FEMALE, BABY , "NOKOTCHI"               },
  { /* 0x02 */ MALE  , CHILD, "KURIBOTCHI"             },
  { /* 0x03 */ MALE  , CHILD, "AHIRUKUTCHI"            },
  { /* 0x04 */ FEMALE, CHILD, "SAKURAMOTCHI"           },
  { /* 0x05 */ FEMALE, CHILD, "MEMEPETCHI"             },
  { /* 0x06 */ MALE  , TEEN , "YOUNGMAMETCHI"          },
  { /* 0x07 */ MALE  , TEEN , "KIKITCHI"               },
  { /* 0x08 */ FEMALE, TEEN , "CHAMAMETCHI"            },
  { /* 0x09 */ FEMALE, TEEN , "ICHIGOTCHI"             },
  { /* 0x0A */ MALE  , ADULT, "MAMETCHI"               },
  { /* 0x0B */ MALE  , ADULT, "KUCHIPATCHI"            },
  { /* 0x0C */ MALE  , ADULT, "GOZARUTCHI"             },
  { /* 0x0D */ MALE  , ADULT, "KUROMAMETCHI"           },
  { /* 0x0E */ MALE  , ADULT, "SHIMASHIMATCHI"         },
  { /* 0x0F */ MALE  , ADULT, "NEMUTCHI"               },
  { /* 0x10 */ MALE  , ADULT, "OYAJITCHI"              },
  { /* 0x11 */ FEMALE, ADULT, "MEMETCHI"               },
  { /* 0x12 */ FEMALE, ADULT, "LOVEZUKINTCHI"          },
  { /* 0x13 */ FEMALE, ADULT, "MAROTCHI"               },
  { /* 0x14 */ FEMALE, ADULT, "VIOLETCHI"              },
  { /* 0x15 */ FEMALE, ADULT, "MAIDTCHI"               },
  { /* 0x16 */ FEMALE, ADULT, "MAKIKO"                 },
  { /* 0x17 */ FEMALE, ADULT, "GRIPPATCHI"             },
  { /* 0x18 */ MALE  , OLDIE, "OJITCHI"                },
  { /* 0x19 */ FEMALE, OLDIE, "OTOKITCHI"              },
  { /* 0x1A */ MALE  , ADULT, "male debug tamagotchi"  },
  { /* 0x1B */ MALE  , ADULT, "PATCHIMAN"              },
  { /* 0x1C */ MALE  , BABY , "KINOTCHI"               },
  { /* 0x1D */ FEMALE, BABY , "NOKOTCHI"               },
  { /* 0x1E */ FEMALE, ADULT, "female debug tamagotchi"},
};

//friend data 
typedef struct {
  byte deviceID;
  byte tamaID;
} friendData;

#define MAX_FRIENDS 14
const friendData friendDataTable[MAX_FRIENDS] = {
  //DevID TamaID | gender << 6
  {0x51,  0x0A }, /* (-｡-) */
  {0x52,  0x0B }, /* (~｡~) */
  {0x53,  0x0C }, /* (…｡…) */
  {0x5B,  0x0D }, /* (・｡・) */ 
  {0x5D,  0x0E }, /* (?｡?) */
  {0x5F,  0x0F }, /* (◯｡◯)*/
  {0x60,  0x10 }, /* (☓｡☓) */
  {0x61,  0x51 }, /* (♥｡♥) */
  {0x62,  0x52 }, /* (☼｡☼) */
  {0x63,  0x53 }, /* (★｡★) */
  {0x64,  0x54 }, /* (@｡@) */
  {0x65,  0x55 }, /* (♪｡♪) */
  {0x66,  0x56 }, /* (╬｡╬) */
  {0x6B,  0x57 }  /* ($｡$) */
};

byte sendBuffer[FRAME_LEN];
byte receiveBuffer[FRAME_LEN];

byte friendNr;
byte buttonMode;
byte ignoreStage = false;

void version(){
  Serial.println("TMGC+C Connect Buddy v1.04 August 2017 by Mr.Blinky");
  Serial.println("Modified to work with Arduino UNO R4 in October 2025 by jojostory");
}

void led_off() {
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_B, LOW);
  digitalWrite(LED_G, LOW);
}

void led_blue() {
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_B, HIGH);
  digitalWrite(LED_G, LOW);
}

void led_red() {
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_B, LOW);
  digitalWrite(LED_G, LOW);
}

void led_magenta() {
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_B, HIGH);
  digitalWrite(LED_G, LOW);
}

void led_green() {
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_B, LOW);
  digitalWrite(LED_G, HIGH);
}

void led_cyan() {
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_B, HIGH);
  digitalWrite(LED_G, HIGH);
}

void led_yellow() {
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_B, LOW);
  digitalWrite(LED_G, HIGH);
}

void led_white() {
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_B, HIGH);
  digitalWrite(LED_G, HIGH);
}


byte receiveFrame() {
  led_blue();
  unsigned long duration;
  duration = pulseIn(IRVO,HIGH,500000);
  led_cyan();
  byte b = 0;
  byte checksum = 0;
  for (int j = 0; j < FRAME_LEN; j++) {
    checksum += b;
    for (int i = 0; i < 8; i++){
      b = b >> 1;
      duration = pulseIn(IRVO,HIGH,TH_TIMEOUT);
      if (duration < TH_0BIT) return false; // frame error
      if (duration >= TH_1BIT) b |= 0x80;
    }
    receiveBuffer[j] = b;
  }
  printFrame(receiveBuffer);
  Serial.print(" Received from ");
  byte id = receiveBuffer[FD_TAMAID] & 0x3F;
  if (id < MAX_TAMAS) Serial.println(tamaDataTable[id].name);
  else  Serial.println(id & 0x3F,HEX);
  if (b == checksum) return true;//success
  else return false; //checksum error
}

void IRpulses(int c) {
  for (int i=0; i < c; i++) {
    digitalWrite(IRLED, HIGH); 
    delayMicroseconds(14); // original: 14
    digitalWrite(IRLED, LOW);
    delayMicroseconds(14); // original: 14
  }
}

void IRwaits(int c) {
  for (int i=0; i < c; i++) delayMicroseconds(14); // original: 14
}

void IRsendByte(byte b) {
  for (byte i=0; i < 8; i++) {
    IRpulses(18);
    if (b & (1 << i)) IRwaits(100);
    else IRwaits (54);
  }
}
  
void sendFrame() { 
  led_yellow();
  byte checksum = 0;
  for (byte i=0 ;i < FRAME_LEN-1; i++) checksum += sendBuffer[i];
  sendBuffer[FRAME_LEN-1] = checksum;
  IRpulses(368); //start pulse
  IRwaits(184);
  for(byte i = 0; i < FRAME_LEN; i++) {
    IRsendByte(sendBuffer[i]);
  }
  IRpulses(46); // end pulse
  delay(1);
  printFrame(sendBuffer);
  Serial.print(" Sent by ");
  byte id = sendBuffer[FD_TAMAID] & 0x3F;
  if (id < MAX_TAMAS) Serial.println(tamaDataTable[id].name);
  else Serial.println(id & 0x3F,HEX);
}
  
byte sendReceiveFrame() { 
  for (byte retries = 0; retries < 5; retries++) {
    sendFrame();
    if (receiveFrame()) return true;
  }
  return false;
}

void makeFrame(byte cmd, byte data) {
  sendBuffer[FD_VERSION] = 0x8D; //TMGC+C ID
  sendBuffer[FD_TYPE]    = cmd;
  sendBuffer[FD_UID_LO]  = 0x80 | (friendDataTable[friendNr].deviceID & 0x3F);
  sendBuffer[FD_TAMAID]  = friendDataTable[friendNr].tamaID;
  sendBuffer[FD_NAME1]   = 0x56;  // '('
  sendBuffer[FD_NAME2]   = friendDataTable[friendNr].deviceID;
  sendBuffer[FD_NAME3]   = 0x55;  // '｡'
  sendBuffer[FD_NAME4]   = friendDataTable[friendNr].deviceID;
  sendBuffer[FD_NAME5]   = 0x57;  // ')' 
  sendBuffer[FD_DATA]    = data;
  for (int i = FD_DATA+1; i < FRAME_LEN-1; i++) sendBuffer[i] = 0;
  //For Tamagotchi P's Yume Kira Bag:
  sendBuffer[0x0C] = (9999 & 0xFF);
  sendBuffer[0x0D] = (9999 >> 8);
  sendBuffer[0x11] = 1;
}
  
void printFrame(byte *buffer) {
  for (int i = 0; i < FRAME_LEN; i++) {
    if (buffer[i] < 16) Serial.print('0');
    Serial.print(buffer[i],HEX);
  }
  //Serial.println();
}

void selectNextFriend() {
  friendNr++;
  if (friendNr >= MAX_FRIENDS) friendNr = 0;
  ignoreStage = false;
}

void setup() {
  randomSeed(analogRead(A0));
  pinMode(IRVO,INPUT);
  pinMode(BUTTON,INPUT_PULLUP);
  pinMode(IRLED,OUTPUT);
  pinMode(LED_R,OUTPUT);
  pinMode(LED_G,OUTPUT);
  pinMode(LED_B,OUTPUT);
  led_red();
  Serial.begin(115200);
  for (byte i = 0; i < 8; i++) { // allow max .8 seconds for serial to get ready
    if (Serial) break;
    delay (100);
  }
  version();
  friendNr = random(10);
}

void loop() {
  led_green();
  byte duration = 0;
  while (digitalRead(BUTTON) == HIGH) {
    if (digitalRead(IRVO) == LOW) {// IR leader pulse must be longer then 5msec
      duration++;
      if (duration >= 100) break;
    } 
    else duration = 0;
    delayMicroseconds(50);
  }
  if (digitalRead(BUTTON) == LOW) {//Button down
    led_blue();
    delay(50);// button debounce delay
    byte buttonDown = 0;
    buttonMode = BM_PLAY;
    while (digitalRead(BUTTON) == LOW) {//wait for button released
      if      (buttonDown >= 231)  buttonDown = 99;
      else if (buttonDown >= 166) {buttonMode = BM_MARRY_GIRL; led_magenta();}
      else if (buttonDown >= 100) {buttonMode = BM_MARRY_BOY; led_cyan();}
      buttonDown++;
      delay(10);
    }
    makeFrame(FT_CONNECT,0);
    if (sendReceiveFrame()) {
      if (buttonMode == BM_PLAY) {
        if (receiveBuffer[0x10] == 0x38) makeFrame(FT_POINTS,1); //Tamagotchi P's give Yume Kira Bag points
        else makeFrame(FT_PLAY,0); //TMGC+C Connect Play
        if (sendReceiveFrame()) selectNextFriend();
      }
      else {
        byte marry = false;
        byte tama  = receiveBuffer[FD_TAMAID] & 0x3F;
        if (tama < MAX_TAMAS) marry = tamaDataTable[tama].stage == ADULT || ignoreStage; //only allow adults to marry unless its the 2nd attempt in a row
        if (marry) {
          makeFrame(FT_MARRY,buttonMode-1);
          if (sendReceiveFrame()) selectNextFriend();
        } else {
          makeFrame(FT_CANCEL,0);
          ignoreStage = sendReceiveFrame();
          for (byte i = 0; i < 6; i++) { //flash LED red for mariage not successful
            led_red(); delay(150);
            led_off(); delay(100);
          }
        }  
      }
    }
  }
  else if (receiveFrame()) {
    delay(25);
    makeFrame(FT_POINTS,0);
    sendFrame();
  }
}

