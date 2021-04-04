#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <HTTPClient.h>

/*********************************************************************************************/
// PRE-COMPILE
/*********************************************************************************************/
#define SS_COM_A  5
#define SS_COM_B 17
#define SS_AD 16
#define SS_EG 4
#define LED 14
#define segA 11
#define segB 12
#define segC 13
#define segD 14
#define segE 15
#define segF 16
#define segG 17

/*********************************************************************************************/
// GLOBAL VARS
/*********************************************************************************************/

//wifi variables
const char* ssid          = "YOUR SSID";
const char* password      = "YOUR PASSWORD";
const char* googleAPI_URL = "YOUR API KEY";

// timing variables
int onTimeDelay = 1;
int settleTime = 0;
int delay_between_common_ABCDEFG = 1;
int delay_between_BW = 1;

// loop variables
int buttonValue;
int previousButtonValue;
int subscribers;
int previousSubscribers = 0;
int i;
int j;

// create buffer for data from HTTP GET request
char payloadBuf[4096];

/*********************************************************************************************/
// FUNCTION PROTOTYPES
/*********************************************************************************************/

int getSubCount(char* payload);
void set_commons_for_white(int display_num);
void set_segments_for_white(int number);
void set_commons_for_black(int display_num);
void set_segments_for_black(int number);
void update_display(int display_num, int value);
void int_to_display(int value);
void individual_segments(uint16_t value);
void allOff(void);
void commonsOff(void);
void segmentsOff(void);

/*********************************************************************************************/
// SET UP
/*********************************************************************************************/

void setup()
{
  // configure GPIO
  pinMode(SS_COM_A, OUTPUT);
  pinMode(SS_COM_B, OUTPUT);
  pinMode(SS_AD, OUTPUT);
  pinMode(SS_EG, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(12, INPUT);
  
  // initialize SPI
  SPI.begin(18, 19, 23, 5); // sck, miso, mosi, ss (ss can be any GPIO)
  
  // initialize SPI slave select lines
  digitalWrite(SS_COM_A, HIGH);
  digitalWrite(SS_COM_B, HIGH);
  digitalWrite(SS_AD, HIGH);
  digitalWrite(SS_EG, HIGH);

  // initialize wifi
  WiFi.begin(ssid, password);
  WiFi.setHostname("YT_SubCounter");
  while(WiFi.status() != WL_CONNECTED)
    delay(500);

  // clear out display
  for(i = 1; i < 7; i++)
  {
    update_display(i, 10);
  }
}


/*********************************************************************************************/
// MAIN
/*********************************************************************************************/

void loop() 
{
  if(WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;

    // configure target server and URL
    http.begin(googleAPI_URL);

    // send HTTP GET request
    int httpCode = http.GET();

    // handle response from server
    if(httpCode > 0)
    {
      // check if GET request has succeeded
      if(httpCode == 200)
      {
        String payload = http.getString();
        payload.toCharArray(payloadBuf, 4096);
        subscribers = getSubCount(&payloadBuf[0]);

        if(subscribers != previousSubscribers)
          int_to_display(subscribers);

        previousSubscribers = subscribers;
        delay(60000*5); // wait 5 minutes after successful update
      }
    }
    else
    {
      // no response... delay 1 second and try again
      delay(10000);
    }
    http.end();
  }

  delay(50);
}


/*********************************************************************************************/
// FUNCTIONS
/*********************************************************************************************/

int getSubCount(char* payload)
{
  int i = 0;
  int j = 0;
  int k;
  int beginIndex;
  int endIndex;
  char charCheck;
  char subcountBuf[8];
  uint8_t subCountLength;
  int subCountReturnValue;

  // begin searching through the payload for "sub"
  while(payload[i] != '\0')
  {
    // search for the character 's'
    if(payload[i] == 's' && payload[i+1] == 'u' && payload[i+2] == 'b')
    {
      // keyword has been found, apply offset to the value of interest
      beginIndex = i+19;

      // search for the end delimiter (double quote)
      charCheck = payload[beginIndex];
      while(charCheck != '"')
      {
        charCheck = payload[beginIndex + j];
        j++;
      }
      // store the end index after the delimiter has been found
      endIndex = beginIndex + j-1;

      //determine how many chars make up the sub count value
      subCountLength = endIndex - beginIndex;

      // copy contents into subscriber count buffer
      for(k=0; k<subCountLength; k++)
      {
        subcountBuf[k] = payload[beginIndex + k];
      }

      // break out of the while loop, value has been captured!
      break;
    }
    i++;
  }

  // convert char array into an interger and return
  subCountReturnValue = atoi(subcountBuf);
  return subCountReturnValue;
}

void allOff(void)
{
  commonsOff();
  segmentsOff();
}

void commonsOff(void)
{
  digitalWrite(SS_COM_A, LOW);
  SPI.transfer((uint8_t)0x00);
  digitalWrite(SS_COM_A, HIGH);  
  digitalWrite(SS_COM_B, LOW);
  SPI.transfer((uint8_t)0x00);
  digitalWrite(SS_COM_B, HIGH);
}

void segmentsOff(void)
{
  digitalWrite(SS_AD, LOW);
  SPI.transfer((uint8_t)0x00);
  digitalWrite(SS_AD, HIGH);
  digitalWrite(SS_EG, LOW);
  SPI.transfer((uint8_t)0x00);
  digitalWrite(SS_EG, HIGH);
}

void set_commons_for_white(int display_num)
{
  uint8_t payload_A = 0;
  uint8_t payload_B = 0;

  switch(display_num)
  {
    case 1:
      payload_A = 0b00000001;
      payload_B = 0b00000000;
      break;
    case 2:
      payload_A = 0b00000010;
      payload_B = 0b00000000;
      break;
    case 3:
      payload_A = 0b00000100;
      payload_B = 0b00000000;
      break;
    case 4:
      payload_A = 0b00001000;
      payload_B = 0b00000000;
      break;
    case 5:
      payload_A = 0b00000000;
      payload_B = 0b00000001;
      break;
    case 6:
      payload_A = 0b00000000;
      payload_B = 0b00000010;
      break;
  }

  digitalWrite(SS_COM_A, LOW);
  SPI.transfer(payload_A);
  digitalWrite(SS_COM_A, HIGH);
  digitalWrite(SS_COM_B, LOW);
  SPI.transfer(payload_B);
  digitalWrite(SS_COM_B, HIGH);
}

void set_segments_for_white(int number)
{
  switch(number)
  {
    case 0:
      individual_segments((uint16_t)0xF030);
      break;

    case 1:
      individual_segments((uint16_t)0x6000);
      break;

    case 2:
      individual_segments((uint16_t)0xB050);
      break;

    case 3:
      individual_segments((uint16_t)0xF040);
      break;

    case 4:
      individual_segments((uint16_t)0x6060);
      break;

    case 5:
      individual_segments((uint16_t)0xD060);
      break;

    case 6:
      individual_segments((uint16_t)0xD070);
      break;

    case 7:
      individual_segments((uint16_t)0x7000);
      break;

    case 8:
      individual_segments((uint16_t)0xF070);
      break;

    case 9:
      individual_segments((uint16_t)0xF060);
      break;

    case 10:
      individual_segments((uint16_t)0x0000);
      break;
  }
}

void set_commons_for_black(int display_num)
{
  uint8_t payload_A = 0;
  uint8_t payload_B = 0;

  switch(display_num)
  {
    case 1:
      payload_A = 0b00010000;
      payload_B = 0b00000000;
      break;
    case 2:
      payload_A = 0b00100000;
      payload_B = 0b00000000;
      break;
    case 3:
      payload_A = 0b01000000;
      payload_B = 0b00000000;
      break;
    case 4:
      payload_A = 0b10000000;
      payload_B = 0b00000000;
      break;
    case 5:
      payload_A = 0b00000000;
      payload_B = 0b00010000;
      break;
    case 6:
      payload_A = 0b00000000;
      payload_B = 0b00100000;
      break;
  }

  digitalWrite(SS_COM_A, LOW);
  SPI.transfer(payload_A);
  digitalWrite(SS_COM_A, HIGH);
  digitalWrite(SS_COM_B, LOW);
  SPI.transfer(payload_B);
  digitalWrite(SS_COM_B, HIGH);
}

void set_segments_for_black(int number)
{
  switch(number)
  {
    case 0:
      individual_segments((uint16_t)0x0004);
      break;

    case 1:
      individual_segments((uint16_t)0x0907);
      break;

    case 2:
      individual_segments((uint16_t)0x0402);
      break;

    case 3:
      individual_segments((uint16_t)0x0003);
      break;

    case 4:
      individual_segments((uint16_t)0x0901);
      break;

    case 5:
      individual_segments((uint16_t)0x0201);
      break;

    case 6:
      individual_segments((uint16_t)0x0200);
      break;

    case 7:
      individual_segments((uint16_t)0x0807);
      break;

    case 8:
      individual_segments((uint16_t)0x0000);
      break;

    case 9:
      individual_segments((uint16_t)0x0001);
      break;
      
    case 10:
      individual_segments((uint16_t)0x0F07);
      break;
  }
}

void update_display(int display_num, int value)
{
  set_commons_for_black(display_num);
  delay(delay_between_common_ABCDEFG);
  set_segments_for_black(value);
  allOff();

  // only update whites when clear case (10) is NOT being sent
  if(value != 10)
  {
    delay(delay_between_BW);
    set_commons_for_white(display_num);
    delay(delay_between_common_ABCDEFG);
    set_segments_for_white(value);
    allOff();
  }
}

void int_to_display(int value)
{
  int convertedVal;
  uint8_t i = 0;
  uint8_t j;
  // convert integer to character array
  char snum[7] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0'};
  itoa(value, &snum[0], 10);

  // figure out how many numbers are in array
  while(snum[i] != '\0')
  {
    i++;
  }

  // right align number in array
  j = 5;
  while(i>0)
  {
    snum[j] = snum[i-1]; // apply appropriate shift
    snum[i-1] = '\0'; // replace with null char
    i--;
    j--;
  }

  // update disp 1
  if(snum[0] == '\0')
    convertedVal = 10;
  else
    convertedVal = snum[0] - '0';
  update_display(1, convertedVal);

  // update disp 2
  if(snum[1] == '\0')
    convertedVal = 10;
  else
    convertedVal = snum[1] - '0';
  update_display(2, convertedVal);

  // update disp 3
  if(snum[2] == '\0')
    convertedVal = 10;
  else
    convertedVal = snum[2] - '0';
  update_display(3, convertedVal);
  
  // update disp 4
  if(snum[3] == '\0')
    convertedVal = 10;
  else
    convertedVal = snum[3] - '0';
  update_display(4, convertedVal);

  // update disp 5
  if(snum[4] == '\0')
    convertedVal = 10;
  else
    convertedVal = snum[4] - '0';
  update_display(5, convertedVal);

  // update disp 6
  if(snum[5] == '\0')
    convertedVal = 10;
  else
    convertedVal = snum[5] - '0';
  update_display(6, convertedVal);
}

void individual_segments(uint16_t value)
{
    uint8_t MSB = 0;
    uint8_t LSB = 0;
    
    MSB = (value & 0xFF00) >> 8;
    LSB = (value & 0x00FF);
    
    // check for a 1 in every position, if one is found, print that one and mask all other ones
    
    int i;
    for(i = 0; i < 8; i++)
    {
        if(MSB & (0x01) << i) // these are A-D
        {
            digitalWrite(SS_AD, LOW);
            SPI.transfer(MSB & (0x01) << i);
            digitalWrite(SS_AD, HIGH);
            delay(onTimeDelay);
            segmentsOff();
            delay(settleTime);
        }
    }
    
    for(i = 0; i < 8; i++)
    {
        if(LSB & (0x01) << i) // these are E - G
        {
            digitalWrite(SS_EG, LOW);
            SPI.transfer(LSB & (0x01) << i);
            digitalWrite(SS_EG, HIGH);
            delay(onTimeDelay);
            segmentsOff();
            delay(settleTime);
        }
    }
}
