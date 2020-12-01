/*
  SD card datalogger
  Log temp and humidity data to an SD card using the SdFat library.
  *****************************************************************
  
  Last updated and working on 11/13/20
  
  used arduino Uno SD card and 16x2 LCD 
  Temp/Humidity sensor is DHT Type 11
  added logging to SD card  11/13
  moved display to new pins to accomadate SD card on SPI  11/13
  changed to SdFat from SD lib  11/13
  Got everything working off timer interupt 11/15
  Rates for sample, display and record to SD are independant now
  Much cleaner LOOP code
  Changed to a Type 22 sensor more accurate
*/

#include <SPI.h>
#include <Wire.h>  // Include Wire Library for I2C

#include <TimeLib.h>
#include <DS1307RTC.h>

DS1307RTC RTC1;

#include <dht_nonblocking.h>
//#define DHT_SENSOR_TYPE DHT_TYPE_11
#define DHT_SENSOR_TYPE DHT_TYPE_22
static const int DHT_SENSOR_PIN = 2;
DHT_nonblocking dht_sensor( DHT_SENSOR_PIN, DHT_SENSOR_TYPE );

#include "SdFat.h"  // switched to better lib
SdFat SD;
#define SD_CS_PIN 4
const int chipSelect = SD_CS_PIN;

#include <LiquidCrystal_I2C.h>  // Display on I2C
// Define I2C Address - change if reqiuired 
const int i2c_addr = 0x27;  // works on my new amazon display
// Define LCD pinout
const int  en = 2, rw = 1, rs = 0, d4 = 4, d5 = 5, d6 = 6, d7 = 7, bl = 3;
LiquidCrystal_I2C lcd(i2c_addr, en, rw, rs, d4, d5, d6, d7, bl, POSITIVE);

// Program data area
float temperature;
float humidity;
String dataString;  // make a string for assembling the data to log:
char *cTime;
File dataFile;     // Data object you will write your sensor data to
const char *sensorFilename = "TempHumidityData.txt";

#define SAMPLETIME 10  // in seconds
#define SDSAVETIME 180  // in seconds

int ticks = 0;  // Variables to count square wave pulses
int old_tick_value = 0;
int ticks2 = 0;

#define DS1307_CTRL_ID 0x68
//#define ledPin LED_BUILTIN
#define ledPin 7

// Set Square Wave on RTC 
void setSQW(uint8_t value) {
  //pinMode (3, INPUT_PULLUP);
  Wire.beginTransmission(DS1307_CTRL_ID);
  Wire.write(7);
  Wire.write(value);
  Wire.endTransmission();
}
 
void handleInt() {  
  // Interupt handler to count seconds in tick vals
  //digitalWrite(ledPin, digitalRead(ledPin) ^ 1);  // flash the LED
  ticks++;
  ticks2++;  
}
 

// ****************************   SETUP    1 TIME CODE   *******************
void setup() {
  Serial.begin( 9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // set up the LCD's number of columns and rows:
  lcd.begin(20, 4);
  lcd.clear();

  //Serial.println(F("Initializing SD card..."));
  lcd.setCursor(0, 0);
  lcd.print(F( "Init SD card.." ));
  delay(2500);

  while (!SD.begin(chipSelect)) {
    delay(1000);
    //Serial.println(F("SD initialization failed!"));
    lcd.setCursor(0, 1);
    lcd.print(F( "SD Init Failed!" ));
  }

  Serial.println(F("SD initialization done."));
  lcd.setCursor(0, 0);
  lcd.print(F( "SD Init. Done!") );
  delay(2000);
  lcd.clear();

  Serial.println(F("Waiting for first sample"));
  lcd.setCursor(0, 0);
  lcd.print( F("Wait for sample.") );

  Serial.println(F("DS1307RTC Set SQW 1 Hz"));
  Serial.println(F("Attach interrupt on D3"));
  Serial.println(F("----------------------"));
 
  pinMode(ledPin,OUTPUT);

  setSQW(0x10);  // // 1Hz  1 second
  
  // D3 on Arduino Uno is hardware interupt 1
  attachInterrupt(1,handleInt,FALLING);
  //attachInterrupt(digitalPinToInterrupt (3),handleInt,CHANGE);

}

/*
 * Poll for a measurement, keeping the state machine alive.  Returns
 * true if a measurement is available.
 */
static bool measure_environment( float *temperature, float *humidity )
{
  static unsigned long measurement_timestamp = millis( );

  /* Measure once every four seconds. */
  if( millis( ) - measurement_timestamp > 2000ul )  // 2 seconds
  {
    if( dht_sensor.measure( temperature, humidity ) == true )
    {
      measurement_timestamp = millis( );
      return( true );
    }
  }

  return( false );
}


char* makeDTStamp(int month, int day, int year, int hour, int min, int sec) {
  static char wavname[40] = {'\0'};
  snprintf(wavname, sizeof(wavname), "%d/%d/%d  %d:%d:%d", month, day, year, hour, min, sec);
  return wavname;
}

void PrintStringToLcd(const char *str) {
    const char *p;
    p = str;
    while (*p) {
        lcd.print(*p);
        p++;
    }
}

void print2digitsToLed(int number) {
  if (number >= 0 && number < 10) {
    lcd.write('0');
  }
  lcd.print(number);
}

void PrintTimeToLed() {
    tmElements_t tm;

    // set the cursor to column 0, line 3
    // (note: line 1 is the second row, since counting begins with 0):
    lcd.setCursor(0, 3);

    if (RTC.read(tm)) 
    {    
      lcd.print(tm.Month);
      lcd.print(F("/"));
      lcd.print(tm.Day);
      lcd.print(F("/"));
      lcd.print(tmYearToCalendar(tm.Year));
      lcd.print(F(" "));
  
      print2digitsToLed(tm.Hour);
      lcd.print(F(":"));
      print2digitsToLed(tm.Minute);
      lcd.print(F(":"));
      print2digitsToLed(tm.Second);
  
      //cTime =  makeDTStamp(tm.Month, tm.Day, tmYearToCalendar(tm.Year), tm.Hour, tm.Minute, tm.Second);
      //Serial.println(cTime);
    }
}


void readDisplayTemp(){
  tmElements_t tm;
  
  /* Measure temperature and humidity and display to LCD.   */
  if( measure_environment( &temperature, &humidity ) == true )
  {
    //Serial.print( "T = " );
    //Serial.print( temperature, 1 );
    //Serial.print( " Deg. C, H = " );
    //Serial.print( humidity, 1 );
    //Serial.println( "%" );
  
    // set the cursor to column 0, line 0
    // (note: line 1 is the second row, since counting begins with 0):
    lcd.setCursor(0, 0);
    
    // print the number of seconds since reset:
    lcd.print( F("Temp  " ));
    lcd.print( ((temperature*9/5)+32), 1 );
    lcd.print( F(" Deg F"));
  
    lcd.setCursor(0, 1);
    lcd.print( F("Humid " ));
    lcd.print( humidity,1);
    lcd.print( F("%" )); 
  
    ticks = 0; // Reset the interval timer
         
  }  // got measurement data 
}

void saveToSD() {
    tmElements_t tm;

    if (RTC.read(tm)) 
 
  // ******************   the data to be sent to logger on SD CARD **************************
  dataString = String(((temperature*9/5)+32)) + ", " + String(humidity) + ", ";
  
  //dataString = "";
  //dataString.concat(cTime);
  //dataString.concat(" ," + String(((temperature*9/5)+32)) + ", " + String(humidity));

  if (RTC.read(tm)){ 
    cTime =  makeDTStamp(tm.Month, tm.Day, tmYearToCalendar(tm.Year), tm.Hour, tm.Minute, tm.Second);
  }
  
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  dataFile = SD.open(sensorFilename, O_CREAT | O_APPEND | O_WRITE);
  
  
  if (dataFile) {  // if the file is available, write to it:
    dataFile.print(dataString);
    dataFile.println(cTime);
    dataFile.flush();
    dataFile.close();
    
    // print to the serial port too:
    //Serial.println(data);
  } 
  else {
    Serial.println("error opening datalog.txt");  // if the file isn't open, pop up an error:
  }

  //digitalWrite(ledPin, digitalRead(ledPin) ^ 1);  // flash the LED-
  ticks2 = 0;  // Reset the interval timer 2
}


void loop() {   
    // Update display if a second has elapsed  
    if (ticks != old_tick_value) {
            old_tick_value = ticks;
            PrintTimeToLed();  // Updates clock on Display
    }

    // Read Temp Humid sensor every sample time seconds and write to SD    
    if (ticks >= SAMPLETIME){
       readDisplayTemp();  // Read Temp Humid sensor every sample time seconds
    }   

    // Save data to SD card in seconds
    if (ticks2 >= SDSAVETIME){
      saveToSD();  // Save data to SD card
    }
}
