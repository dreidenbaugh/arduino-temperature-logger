// Dietrich Reidenbaugh
// June 2016

#include <LiquidCrystal.h>
#include <math.h>
#include <EEPROM.h>

int samplingPeriod = 240; // Sampling frequency in seconds

// Pin numbers:
const int pin_TempSensor = A0; // Pin for temperature sensor
const int pin_LCD_DB7 = 2; // Pin for LCD data pin 7
const int pin_LCD_DB6 = 3; // Pin for LCD data pin 6
const int pin_LCD_DB5 = 4; // Pin for LCD data pin 5
const int pin_LCD_DB4 = 5; // Pin for LCD data pin 4
const int pin_LED_1 = 6; // Pin for LED 1
const int pin_LED_2 = 7; // Pin for LED 2
const int pin_Button_L = 8; // Pin for left button
const int pin_Button_R = 9; // Pin for right button
const int pin_LCD_BL1 = 10; // Pin for LCD backlight
const int pin_LCD_EN = 11; // Pin for LCD enable
const int pin_LCD_RS = 12; // Pin for LCD register select
const int pin_Piezo = 13; // Pin for piezo

LiquidCrystal lcd(pin_LCD_RS, pin_LCD_EN, pin_LCD_DB4, pin_LCD_DB5, pin_LCD_DB6, pin_LCD_DB7); // LCD interface pin numbers

long loopNumber = -1;
long secondsElapsed = -1;
long secondsElapsedAtLastLoop;
boolean firstLoopOfSecond;
long minutesElapsed = - 1;
int hoursElapsed = -1;

int LED_1_Enabled;
int LED_2_Enabled;
int button_L_Pressed;
int button_R_Pressed;
int LCD_Backlight_Enabled;

int button_L_Time = 0;
int button_R_Time = 0;
int button_L_Time_Previous;
int button_R_Time_Previous;

float tempSensorValue;
float vcc; // Arduino voltage supply in mV

float currentTemperature;

const int recentTemperaturesLength = 30;
float recentTemperatures[recentTemperaturesLength] = { NULL };
float recentTemperaturesMedian;

const byte recordedTemperaturesLength = 500;
byte recordedTemperatures[recordedTemperaturesLength] = { NULL };
int currentRecordIndex = 0;

char displayMode = 'l';
int displayIndex = 0;

void setup() {
  Serial.begin(9600);
  
  Serial.print("Time: ");
  Serial.print(millis() / 1000.0);
  Serial.print(" | ");
  Serial.print("Sampling Period: ");
  Serial.print(samplingPeriod);
  Serial.println(" seconds");
  
  pinMode(pin_LED_1, OUTPUT);
  pinMode(pin_LED_2, OUTPUT);
  pinMode(pin_Button_L, INPUT);
  pinMode(pin_Button_R, INPUT);
  pinMode(pin_LCD_BL1, OUTPUT);
  pinMode(pin_Piezo, OUTPUT);

  LED_1_Enabled = 0;
  digitalWrite(pin_LED_1, LOW);
  LED_2_Enabled = 0;
  digitalWrite(pin_LED_2, LOW);
  LCD_Backlight_Enabled = 1;
  digitalWrite(pin_LCD_BL1, HIGH);
  
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
}

void loop() {
  loopNumber++;
  secondsElapsedAtLastLoop = secondsElapsed;
  secondsElapsed = millis() / 1000.0;
  if(secondsElapsed != secondsElapsedAtLastLoop)
  {
    firstLoopOfSecond = 1;
  }
  else
  {
    firstLoopOfSecond = 0;
  }
  minutesElapsed = secondsElapsed / 60;
  hoursElapsed = minutesElapsed / 60;
  
  // Every cycle,
  if(loopNumber % 1 == 0)
  {
    button_L_Pressed = digitalRead(pin_Button_L);
    button_R_Pressed = digitalRead(pin_Button_R);
    
    if(button_L_Pressed)
    {
      button_L_Time++;
    }
    else
    {
      button_L_Time = 0;
    }
    
    if(button_R_Pressed)
    {
      button_R_Time++;
    }
    else
    {
      button_R_Time = 0;
    }
    
    if(displayMode == 'l')
    {
      if(button_R_Time == 0 && button_R_Time_Previous >= 1 && button_R_Time_Previous <= 50)
      {
        displayMode = 'd';
        displayIndex = 0;
      }
    }
    else if(displayMode == 'd')
    {
      if((button_L_Time == 0 || button_R_Time == 0) && button_L_Time_Previous >= 300 && button_R_Time_Previous >= 300)
      {
        if(displayIndex == -3)
        {
          Serial.print("Time: ");
          Serial.print(millis() / 1000.0);
          Serial.println(" | Starting Write to EEPROM");
          writeToEEPROM();
          Serial.print("Time: ");
          Serial.print(millis() / 1000.0);
          Serial.println(" | Ending Write to EPROM");
        }
        else if(displayIndex == -2)
        {
          Serial.print("Time: ");
          Serial.print(millis() / 1000.0);
          Serial.println(" | Starting Read from EEPROM");
          readFromEEPROM();
          Serial.print("Time: ");
          Serial.print(millis() / 1000.0);
          Serial.println(" | Ending Read from EPROM");
        }
      }
      else if((button_L_Time == 0 || button_R_Time == 0) && button_L_Time_Previous >= 50 && button_R_Time_Previous >= 50)
      {
        if(displayIndex == -1)
        {
          Serial.print("Time: ");
          Serial.print(millis() / 1000.0);
          Serial.println(" | Starting Read from SRAM");
          readFromSRAM();
          Serial.print("Time: ");
          Serial.print(millis() / 1000.0);
          Serial.println(" | Ending Read from SRAM");
        }
      }
      else if(button_R_Time == 0 && button_R_Time_Previous >= 1 && button_R_Time_Previous <= 50)
      {
        displayIndex++;
      }
      else if(button_R_Time >= 100 && button_L_Time == 0)
      {
        displayMode = 'l';
      }
      else if(button_L_Time == 0 && button_L_Time_Previous >= 1 && button_L_Time_Previous <= 50)
      {
        if(displayIndex > -3)
        {
          displayIndex--;
        }
      }
    }
    
    button_L_Time_Previous = button_L_Time;
    button_R_Time_Previous = button_R_Time;
  }
  
  // Every 30 seconds,
  if(secondsElapsed % 30 == 0 && firstLoopOfSecond == 1)
  {
    updateVcc();
    Serial.print("Time: ");
    Serial.print(millis() / 1000.0);
    Serial.print(" | ");
    Serial.print("Arduino Power Supply: ");
    Serial.print(vcc);
    Serial.println(" mV");
  }
  
  // Every 75 cycles,
  if(loopNumber % 75 == 0)
  {
    tempSensorValue = analogRead(pin_TempSensor);
    updateTemperature();
    Serial.print("Time: ");
    Serial.print(millis() / 1000.0);
    Serial.print(" | ");
    Serial.print("Current Temperature: ");
    Serial.print(currentTemperature);
    Serial.print(" | Median Temperature: ");
    Serial.println(recentTemperaturesMedian);
    LED_1_Enabled = 1;
  }
  
  // Every 75 cycles, delayed by 20 cycles;
  if((loopNumber - 20) % 75 == 0)
  {
    LED_1_Enabled = 0;
  }
  
  // Except in the first second,
  if(secondsElapsed != 0)
  {
    // Every (samplingPeriod) seconds,
    if(secondsElapsed % (samplingPeriod) == 0 && firstLoopOfSecond == 1)
    {
      LED_2_Enabled = 1;
      currentRecordIndex++;
      recordedTemperatures[currentRecordIndex] = round(recentTemperaturesMedian);
      Serial.print("Time: ");
      Serial.print(millis() / 1000.0);
      Serial.print(" | ");
      Serial.print("Sample ");
      Serial.print(currentRecordIndex);
      Serial.print(": ");
      Serial.println(recordedTemperatures[currentRecordIndex]);
    }
  
    // Every (samplingPeriod) seconds, delayed by 1 second,
    if((secondsElapsed - 1) % (samplingPeriod) == 0 && firstLoopOfSecond == 1)
    {
      LED_2_Enabled = 0;
    }
  }
  
  // Every 5 cycles,
  if(loopNumber % 5 == 0)
  {
    updateScreen();
  }
  
  updateDigitalOutputs();
  
  delay(10);
}

void updateTemperature()
{
  // Calculate current temperature
  float voltage = (tempSensorValue / 1024.0) * vcc / 1000.0;
  currentTemperature = (voltage - 0.5) * 100.0 * 9.0 / 5.0 + 32.0;
  
  // Add to recentTemperatures after shifting old data while counting data points
  int recentTemperaturesNumber = 0;
  for(int i = recentTemperaturesLength - 2; i >= 0; i--)
  {
    if(recentTemperatures[i])
    {
      recentTemperaturesNumber++;
      recentTemperatures[i + 1] = recentTemperatures[i];
    }
  }
  recentTemperaturesNumber++;
  recentTemperatures[0] = currentTemperature;
  recentTemperaturesMedian = median(recentTemperatures, recentTemperaturesNumber);
  
  return;
}

void updateScreen()
{
  if(displayMode == 'l')
  {
    lcd.setCursor(0, 0);
    lcd.print(hoursElapsed);
    lcd.print(":");
    if ((minutesElapsed % 60) < 10)
    {
      lcd.print("0");
    }
    lcd.print(minutesElapsed % 60);
    lcd.print(":");
    if ((secondsElapsed % 60) < 10)
    {
      lcd.print("0");
    }
    lcd.print(secondsElapsed % 60);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(round(recentTemperaturesMedian));
    lcd.print((char)223);
    lcd.print("F");
    lcd.print("                ");
  }
  if(displayMode == 'd')
  {
    if(displayIndex == -3)
    {
      lcd.setCursor(0, 0);
      lcd.print("Write to EEPROM");
      lcd.print("                ");
      lcd.setCursor(0, 1);
      if(button_L_Time >= 300 && button_R_Time >= 300)
      {
        lcd.print("Release");
      }
      else if(button_L_Time >= 1 && button_R_Time >= 1)
      {
        lcd.print("Continue Holding");
      }
      else
      {
        lcd.print("Hold L and R");
      }
      lcd.print("                ");
    }
    else if(displayIndex == -2)
    {
      lcd.setCursor(0, 0);
      lcd.print("Read from EEPROM");
      lcd.print("                ");
      lcd.setCursor(0, 1);
      if(Serial)
      {
        if(button_L_Time >= 300 && button_R_Time >= 300)
        {
          lcd.print("Release");
        }
        else if(button_L_Time >= 1 && button_R_Time >= 1)
        {
          lcd.print("Continue Holding");
        }
        else
        {
          lcd.print("Hold L and R");
        }
      }
      else
      {
        lcd.print("No Connection");
      }
      lcd.print("                ");
    }
    else if(displayIndex == -1)
    {
      lcd.setCursor(0, 0);
      lcd.print("Read from SRAM");
      lcd.print("                ");
      lcd.setCursor(0, 1);
      if(Serial)
      {
        if(button_L_Time >= 50 && button_R_Time >= 50)
        {
          lcd.print("Release");
        }
        else if(button_L_Time >= 1 && button_R_Time >= 1)
        {
          lcd.print("Continue Holding");
        }
        else
        {
          lcd.print("Hold L and R");
        }
      }
      else
      {
        lcd.print("No Connection");
      }
      lcd.print("                ");
    }
    else if(displayIndex == 0)
    {
      lcd.setCursor(0, 0);
      lcd.print("Data View");
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print("Samp. Per.: ");
      lcd.print(samplingPeriod);
      lcd.print("                ");
    }
    else if(displayIndex > 0)
    {
      lcd.setCursor(0, 0);
      lcd.print("Sample: ");
      lcd.print(displayIndex);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      if(recordedTemperatures[displayIndex])
      {
        lcd.print("Temp: ");
        lcd.print(recordedTemperatures[displayIndex]);
        lcd.print((char)223);
        lcd.print("F");
      }
      else
      {
        lcd.print("No Data");
      }
      lcd.print("                ");
    }
  }
  if(displayMode == 's')
  {
    lcd.setCursor(0, 0);
    lcd.print("[TEST] Settings");
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
  }
  return;
}

void updateDigitalOutputs()
{
    if(LED_1_Enabled == 1)
  {
    digitalWrite(pin_LED_1, HIGH);
  }
  else
  {
    digitalWrite(pin_LED_1, LOW);
  }
  
  if(LED_2_Enabled == 1)
  {
    digitalWrite(pin_LED_2, HIGH);
  }
  else
  {
    digitalWrite(pin_LED_2, LOW);
  }
  
  if(LCD_Backlight_Enabled == 1)
  {
    digitalWrite(pin_LCD_BL1, HIGH);
  }
  else
  {
    digitalWrite(pin_LCD_BL1, LOW);
  }
  return;
}

void updateVcc() {
  // from http://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both

  long result = (high<<8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  
  vcc = result; // Vcc in millivolts
  return;
}

void readFromSRAM()
{
  for(int i = 1; i <= currentRecordIndex; i++)
  {
    Serial.print(i);
    Serial.print(",");
    Serial.println(recordedTemperatures[i]);
  }
}

void readFromEEPROM()
{
  for(int i = 1; i <= recordedTemperaturesLength; i++)
  {
    byte value = EEPROM.read(i);
    if(value < 200)
    {
      Serial.print(i);
      Serial.print(",");
      Serial.println(value, DEC);
    }
    else
    {
      break;
    }
  }
}

void writeToEEPROM()
{
  int i;
  for(i = 1; i <= currentRecordIndex; i++)
  {
    EEPROM.write(i, recordedTemperatures[i]);
  }
  EEPROM.write(i, 255);
}

float median(float input[], int n)
{
  float temp;
  int i, j;
  float x[n];
  for(i = 0; i < n; i++)
  {
    x[i] = input[i];
  }
  // Sort the data
  for(i = 0; i < n - 1; i++)
  {
    for(j = i + 1; j < n; j++)
    {
      if(x[j] < x[i])
      {
        temp = x[i];
        x[i] = x[j];
        x[j] = temp;
      }
    }
  }
  // If n is even, return the mean of the two middle elements
  if(n % 2 == 0)
  {
    return((x[n / 2] + x[n / 2 - 1]) / 2.0);
  }
  // Otherwise, return the middle element
  else
  {
    return x[n / 2];
  }
}
