#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
// RTC stuff from ds3231 example
#include <SPI.h>
#include <RTClib.h>
#include <RTC_DS3231.h>

RTC_DS3231 RTC;
#define SQW_FREQ DS3231_SQW_FREQ_1024     //0b00001000   1024Hz
#define PWM_COUNT 1020   //determines how often the LED flips
#define LOOP_DELAY 5000 //ms delay time in loop

#define RTC_SQW_IN 5     // input square wave from RTC into T1 pin (D5)
                         // WE USE TIMER1 so that it does not interfere with Arduino delay() command
#define INT0_PIN   2     // INT0 pin for 32kHz testing?
#define LED_PIN    9     // random LED for testing...tie to ground through series resistor..
#define LED_ONBAORD 13   // Instead of hooking up an LED, the nano has an LED at pin 13.
#define DST_SWITCH 6 //Pin 7 is connected to the daylight savings time switch

//----------- GLOBALS  -------------------------

volatile long TOGGLE_COUNT = 0;
    
ISR(TIMER1_COMPA_vect) {
  //digitalWrite(LED_PIN, !digitalRead(LED_PIN));      // ^ 1);
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  digitalWrite(LED_ONBAORD, !digitalRead(LED_ONBAORD)); //useful on nano's and some other 'duino's
  TOGGLE_COUNT++;
}    

ISR(INT0_vect) {
  // Do something here
  //digitalWrite(LED_PIN, !digitalRead(LED_PIN));      // ^ 1);
   //TOGGLE_COUNT++;
}

// end RTC stuff

Adafruit_7segment matrix = Adafruit_7segment();
Adafruit_7segment matrix1 = Adafruit_7segment();
Adafruit_7segment matrixBig = Adafruit_7segment();
Adafruit_BicolorMatrix matrix8 = Adafruit_BicolorMatrix();
Adafruit_24bargraph tempBar = Adafruit_24bargraph();
Adafruit_24bargraph baroBar = Adafruit_24bargraph();


/* This driver uses the Adafruit unified sensor library (Adafruit_Sensor),
   which provides a common 'type' for sensor data and some helper functions.
   
   To use this driver you will also need to download the Adafruit_Sensor
   library and include it in your libraries folder.

   You should also assign a unique ID to this sensor for use with
   the Adafruit Sensor API so that you can identify this particular
   sensor in any data logs, etc.  To assign a unique ID, simply
   provide an appropriate value in the constructor below (12345
   is used by default in this example).
   
   Connections
   ===========
   Connect SCL to analog 5
   Connect SDA to analog 4
   Connect VDD to 3.3V DC
   Connect GROUND to common ground
    
   History
   =======
   2013/JUN/17  - Updated altitude calculations (KTOWN)
   2013/FEB/13  - First version (KTOWN)
*/
   
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

/**************************************************************************/
/*
    Displays some basic information on this sensor from the unified
    sensor API sensor_t type (see Adafruit_Sensor for more information)
*/
/**************************************************************************/
void displaySensorDetails(void)
{
  sensor_t sensor;
  bmp.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" hPa");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" hPa");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" hPa");  
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

/**************************************************************************/
/*
    Arduino setup function (automatically called at startup)
*/
/**************************************************************************/

float temp[8];
float baro[8];
int blocks[8];
float temperature;
float tempF;
int firstTime=1;
int c;
sensors_event_t event;
void setup(void) 
{
  Serial.begin(9600);
  Serial.println("Pressure Sensor Test"); Serial.println("");
  
  /* Initialise the sensor */
  if(!bmp.begin())
  {
    /* There was a problem detecting the BMP085 ... check your connections */
    Serial.print("Ooops, no BMP085 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
  pinMode(DST_SWITCH,INPUT_PULLUP);
  /* Display some basic information on this sensor */
  displaySensorDetails();
  // initialize the various LED toys with their serial addresses
  matrix.begin(0x71);
  matrixBig.begin(0x72);
  matrixBig.setBrightness(12);
  matrix1.begin(0x70);
  tempBar.begin(0x73);
  matrix8.begin(0x74);  // pass in the address
  baroBar.begin(0x75);
  matrix8.clear();      // clear display
  for (c=0;c<8;c++)
  	matrix8.drawPixel(c, 4, LED_GREEN); //Draw the initial temp trend line
  matrix8.writeDisplay();  // write the changes we just made to the display
  //set the bar graph to all off
  for (c=0;c<24;c++)
  	tempBar.setBar(c,LED_OFF);
  tempBar.writeDisplay();

  bmp.getEvent(&event);
  bmp.getTemperature(&temperature);
  tempF=((temperature*9/5)+32);
  for (int x=0; x<9; x++) {
    temp[x]=tempF;
    blocks[x]=0;
  }
//Real Time Clock  stuff
    pinMode(RTC_SQW_IN, INPUT);
    pinMode(INT0_PIN, INPUT);
    if (! RTC.isrunning()) {
      Serial.println("RTC is NOT running!");
      // following line sets the RTC to the date & time this sketch was compiled
      RTC.adjust(DateTime(__DATE__, __TIME__));
    }
  
    DateTime now = RTC.now();
    DateTime compiled = DateTime(__DATE__, __TIME__);
    if (now.unixtime() < compiled.unixtime()) {
      //Serial.println("RTC is older than compile time!  Updating");
      RTC.adjust(DateTime(__DATE__, __TIME__));
    }
    
    RTC.enable32kHz(true);
    RTC.SQWEnable(true);
    RTC.BBSQWEnable(true);
    RTC.SQWFrequency( SQW_FREQ );
  
    char datastr[100];
    RTC.getControlRegisterData( datastr[0]  );
    Serial.print(  datastr );
 
  
  
    //--------INT 0---------------
    EICRA = 0;      //clear it
    EICRA |= (1 << ISC01);
    EICRA |= (1 << ISC00);   //ISC0[1:0] = 0b11  rising edge INT0 creates interrupt
    EIMSK |= (1 << INT0);    //enable INT0 interrupt
        
    //--------COUNTER 1 SETUP -------
    setupTimer1ForCounting((int)PWM_COUNT); 
    printTimer1Info();   
//end RTC stuff
}

void loop(void) 
{
    DateTime now = RTC.now();
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    Serial.print(now.hour()*100+now.minute());
 
 
 
  /* Get a new sensor event */ 
  
  bmp.getEvent(&event);
 
  /* Display the results (barometric pressure is measure in hPa) */
  if (event.pressure)
  {
    /* Display atmospheric pressue in hPa */
    Serial.print("  Pressure:    ");
    Serial.print(event.pressure/33.86);
    Serial.print(" in");
 	Serial.print("  DST Switch- ");

     
    /* First we get the current temperature from the BMP085 */
    
    bmp.getTemperature(&temperature);
    Serial.print("  Temperature: ");
    Serial.print(temperature);
    Serial.print(" C");

    /* Then convert the atmospheric pressure, and SLP to altitude         */
    /* Update this next line with the current SLP for better results      */
    float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
    Serial.print("  Altitude:    "); 
    Serial.print(bmp.pressureToAltitude(seaLevelPressure,
                                        event.pressure)); 
    Serial.print(" m  ");
    Serial.println("");
  }
  else
  {
    Serial.println("Sensor error");
  }
  //
  // Deal with temp and pressure displays
  //
  float pressureInches=event.pressure/33.86; //convert to inches of mercury
  matrix1.print(pressureInches); //print barometric pressure
  tempF=((temperature*9/5)+32); //Let's use Farenheit
  matrix.print(tempF); //set the temperature display
  matrix.writeDigitRaw(4,113); //add the "F" at the end
  matrix1.writeDisplay();
  matrix.writeDisplay();
  matrix8.writeDisplay();
  //
  // Now the time
  //
  int dst=(digitalRead(DST_SWITCH));
  int timeNow=now.hour()*100+now.minute();
  if (dst==1) timeNow=timeNow+100;
  if (timeNow>1259) timeNow=timeNow-1200;
  //if (readPin(DST_SWITCH)==1) timeNow=timeNow+1;
  matrixBig.print(timeNow,DEC);
  matrixBig.drawColon(1);
  if (now.hour()>11)
  	matrixBig.writeDigitRaw(2,0x06);
  if (now.hour()==0) {
  	matrixBig.writeDigitNum(0,0);
  	matrixBig.writeDigitNum(1,0);
  }
  matrixBig.writeDisplay();
  //
  //set the bar graphs to all off
  for (c=0;c<24;c++)
  	baroBar.setBar(c,LED_OFF);
  for (c=0;c<24;c++)
  	tempBar.setBar(c,LED_OFF);
 //
 //paint the pixels for the temperature bar
 //
  for (c=0;c<(tempF-61);c++)
  	if ((tempF-61)<6)
  		tempBar.setBar(23-c,LED_YELLOW);
  	else if ((tempF-61)>19)
  		tempBar.setBar(23-c,LED_RED);
  	else tempBar.setBar(23-c,LED_GREEN);
  tempBar.writeDisplay();
//
//now the pressure bar
//
int baroElements= round((pressureInches-29)*10);
Serial.print("  baroElements ");
Serial.print(baroElements);
 /* for (c=0;c<baroElements;c++)
  	if (baroElements<6)
  		baroBar.setBar(32-c,LED_RED);
  	else
  		baroBar.setBar(23-c,LED_YELLOW);
 */
 baroBar.setBar(baroElements,LED_RED);
 baroBar.writeDisplay();
//
//
// move the trend graph
//
  int secs=now.second();
  if ((secs==0) && (firstTime==1)) {
    for (int x=0; x<8; x++ ){
      temp[x]=temp[x+1];
      baro[x]=baro[x+1];
      blocks[x]=blocks[x+1];
    
    }
    temp[7]=tempF;
  
    blocks[7]=temp[7]-temp[6];
    matrix8.clear();
    for (int x=0; x<8; x++ ){
      if (blocks[x]==0) {
        matrix8.drawPixel(x,4,LED_GREEN);
      } else {
          if (blocks[x]>0) {
            matrix8.drawPixel(x,4+blocks[x],LED_GREEN);
          //  blocks[x]--;
          }
          if (blocks[x]<0) {
            matrix8.drawPixel(x,4-blocks[x],LED_GREEN);
          //  blocks[x]++;
          }
       }
	Serial.print("x ");
	Serial.print(x);
	Serial.print(" blocks[x] ");
	Serial.print(blocks[x]); 
	Serial.print(" temp[x] ");
	Serial.println(temp[x]); 
}
//matrix8.writeDisplay();
    firstTime=0;
}
  if (secs==1) firstTime=1;
  delay(500);
}
//==========================end void()

//#########################################################
// TIMER/COUNTER CONTROLS 
//   Timer 0 is PD4
//   Timer 1 is PD5
//#########################################################

/**
* Setup Timer 0 to count and interrupt
*
* --- TCCR0A ---
* COM0A1  COM0A0  COM0B1 COM0B0  xxx  xxx  WGM01  WGM00
* --- TCCR0B ---
* FOC0A   FOC0B   xxx     xxx    WGM02 CS02  CS01  CS00
*
*    In non-PWM Mode COM0A1:0 == 0b00  Normal port operation, OC0A disconnected
*        same for COM0B1:0   
* For WGM02 WGM01 WGM00 
*  0b000  Normal mode, with a TOP of 0xFF and update of OCRx immediately
*           Counting is upwards.  No counter clear performed.  Simply overruns and restarts.
*  0b010  CTC Mode with top of OCRA and update of OCRx immediately.
*        Clear Timer on Compare Match, OCR0A sets top.  Counter is cleared when TCNT0 reaches OCR0A
*        Need to set the OCF0A interrupt flag
*
* FOC0A/FOC0B should be set zero. In non-PWM mode it's a strobe. 
*        But does not generate any interrupt if using CTC mode.
* 
* CS02:0  Clock Select:
*   0b000  No clock source. Timer/Counter stopped
*   0b110  External clock on T0 pin FALLING EDGE
*   0b111  External clock on T0 pin RISING EDGE
*
* TCNT0  Timer counter register
*
* OCR0A/OCR0B  Output Compare Register
*
* --- TIMSK0 ---
* xxx xxx xxx xxx xxx OCIE0B  OCIE0A  TOIE0
*    OCIE0B/A are the Output Compare Interrupt Enable bits
*    TOIE0 is the Timer Overflow Interrup Enable
*
*  Must write OCIE0A to 1 and then I-bit in Status Register (SREG)
*
* --- TIFR0 ---   Timer Counter Interrupt Flag Register
*  xx xx xx xx   xx OCF0B OCF0A TOV0
*   OCF0B/A  Output Compare A/B Match Flag 
*          is set when a match occurs.  Cleared by hardware when executing interrupt.
*          Can also be cleared by writing a 1 to the flag.
*
*/
void setupTimer0ForCounting(uint8_t count) {
  //set WGM2:0 to 0b010 for CTC
  //set CS02:0 to 0b111 for rising edge external clock

  TCCR0A = 0;
  TCCR0B = 0;
  TIMSK0 = 0;
 
  TCCR0A |= (1 << WGM01);
//  TCCR0A = _BV(WGM01);

  TCCR0B |= (1 << CS02);
  TCCR0B |= (1 << CS01);
  TCCR0B |= (1 << CS00);

//  TCCR0B = _BV(CS02) || _BV(CS01) || _BV(CS00);
  
  TCNT0 = 0;
  
  OCR0A = count;      // SET COUNTER 
  
 // TIMSK0 = _BV(OCIE0A);   // SET INTERRUPTS
  TIMSK0 |= (1 << OCIE0A);
}

/**
* @function: setupTimer1ForCounting
* @param count  16-bit integer to go into OCR1A
*
* TCCR1A = [ COM1A1, COM1A0, COM1B1, COM1B0, xxx, xxx, WGM11, WGM10]
* TCCR1B = [ ICNC1,  ICES1,  xxx,    WGM13, WGM12, CS12, CS11, CS10]
* TCCR1C = [ FOC1A,  FOC1B, xxx, xxx, xxx, xxx, xxx, xxx]
* TIMSK1 = [ xxxx,  xxxx,  ICIE1,  xxxx, xxxx, OCIE1B, OCIE1A, TOIE1]
*
*  Set COM1A, COM1B to 0 for normal operation with OC1A/OC1B disonnected.
*  Set WGM13:0 to 0b0100 for CTC Mode using OCR1A
*
*  We won't use the Input Capture Noise Canceler (ICNC1)
*  CS12:0 to 0b111 for external source on T1, clock on rising edge.
*
*  TCNT1H and TCNT1L  (TCNT1) 
*  OCR1AH / OCR1AL  Output Compare Register 1A
*       Can we set this by controlling OCR1A only?  Or do we need to bit shift.
*
*  Set OCIE1A
*/
void setupTimer1ForCounting(int count) {
  //set WGM1[3:0] to 0b0100 for CTC mode using OCR1A. Clear Timer on Compare Match, OCR1A sets top. 
  //                            Counter is cleared when TCNT0 reaches OCR0A
  //set CS1[2:0] to 0b111 for external rising edge T1 clocking.
  //set OCR1A to count
  //set TIMSK1 to OCIE1A

  //clear it out
  TCCR1A = 0;      //nothing else to set
  TCCR1B = 0;
  TIMSK1 = 0;
 
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS12);
  TCCR1B |= (1 << CS11);
  TCCR1B |= (1 << CS10);  
  
  TCNT1 = 0;
  
  OCR1A = count;      // SET COUNTER 
  
  TIMSK1 |= (1 << OCIE1A);
}


void printTimer0Info() {
  Serial.println(" ");  
  Serial.println("----- Timer1 Information -----");
  Serial.print("TCCR0A: ");
  Serial.println(TCCR0A, BIN);
  Serial.print("TCCR0B: ");
  Serial.println(TCCR0B, BIN);
  Serial.print("TIMSK0: ");
  Serial.println(TIMSK0, BIN);
  Serial.println("------------------------------");
  Serial.println(" ");  
}


void printTimer1Info() {
  Serial.println(" ");  
  Serial.println("----- Timer1 Information -----");
  Serial.print("TCCR1A: ");
  Serial.println(TCCR1A, BIN);
  Serial.print("TCCR1B: ");
  Serial.println(TCCR1B, BIN);
  Serial.print("TIMSK1: ");
  Serial.println(TIMSK1, BIN);
  Serial.print("OCR1A: " );
  Serial.println(OCR1A, BIN);
  Serial.println("------------------------------");
  Serial.println(" ");  
}
// vim:ci:sw=4 sts=4 ft=cpp
