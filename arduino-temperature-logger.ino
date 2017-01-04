// Dietrich Reidenbaugh
// Updated January 2017

#include <LiquidCrystal.h>
#include <math.h>
#include <EEPROM.h>

int sampling_period = 240; // Sampling period in seconds

// Pin numbers:
const int pin_temp_sensor = A0; // Pin for temperature sensor
const int pin_lcd_db7 = 2; // Pin for LCD data pin 7
const int pin_lcd_db6 = 3; // Pin for LCD data pin 6
const int pin_lcd_db5 = 4; // Pin for LCD data pin 5
const int pin_lcd_db4 = 5; // Pin for LCD data pin 4
const int pin_led_1 = 6; // Pin for LED 1
const int pin_led_2 = 7; // Pin for LED 2
const int pin_button_left = 8; // Pin for left button
const int pin_button_right = 9; // Pin for right button
const int pin_lcd_bl1 = 10; // Pin for LCD backlight
const int pin_lcd_en = 11; // Pin for LCD enable
const int pin_lcd_rs = 12; // Pin for LCD register select
const int pin_piezo = 13; // Pin for piezo

LiquidCrystal lcd(pin_lcd_rs, pin_lcd_en, pin_lcd_db4, pin_lcd_db5,
                  pin_lcd_db6, pin_lcd_db7); // LCD interface pin numbers

// Timekeeping variables:
long loop_number = -1;
long seconds_elapsed = -1;
long seconds_elapsed_last_loop;
boolean is_first_loop_of_second;
long minutes_elapsed = -1;
int hours_elapsed = -1;
byte loops_since_action = 0;

// Settings variables:
boolean lcd_backlight_enabled;
boolean sound_enabled = false;

// Button variables: 
boolean button_left_pressed;
boolean button_right_pressed;
unsigned int button_left_time = 0;
unsigned int button_right_time = 0;
unsigned int button_left_time_previous;
unsigned int button_right_time_previous;

// LED variables:
boolean led_1_enabled;
boolean led_2_enabled;

// Display varibles:
char display_mode = 'l';
int display_index = 0;

// Arduino supply voltage variable:
float supply_voltage; // units are millivolts (mV)

// Array variables:
const int recent_temperatures_length = 30;
float recent_temperatures[recent_temperatures_length] = { NULL };
float recent_temperatures_median;
const int recorded_temperatures_length = 500;
byte recorded_temperatures[recorded_temperatures_length] = { NULL };
int current_record_index = 0;

void setup() {
  Serial.begin(9600);

  // Print the sampling period to the serial port
  Serial.print("Time: ");
  Serial.print(millis() / 1000.0);
  Serial.print(" | ");
  Serial.print("Sampling Period: ");
  Serial.print(sampling_period);
  Serial.println(" seconds");

  // Set up the pin modes
  pinMode(pin_led_1, OUTPUT);
  pinMode(pin_led_2, OUTPUT);
  pinMode(pin_button_left, INPUT);
  pinMode(pin_button_right, INPUT);
  pinMode(pin_lcd_bl1, OUTPUT);
  pinMode(pin_piezo, OUTPUT);

  // Set the initial digital output states
  led_1_enabled = 0;
  digitalWrite(pin_led_1, LOW);
  led_2_enabled = 0;
  digitalWrite(pin_led_2, LOW);
  lcd_backlight_enabled = 1;
  digitalWrite(pin_lcd_bl1, HIGH);

  // Print "Initializing..." to the LCD
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
}

void loop() {
  // Update timekeeping variables
  loop_number++;
  seconds_elapsed_last_loop = seconds_elapsed;
  seconds_elapsed = millis() / 1000.0;
  if (seconds_elapsed != seconds_elapsed_last_loop)
  {
    is_first_loop_of_second = 1;
  }
  else
  {
    is_first_loop_of_second = 0;
  }
  minutes_elapsed = seconds_elapsed / 60;
  hours_elapsed = minutes_elapsed / 60;
  if (loops_since_action < 255) // 255 is maximum value for byte
  {
    loops_since_action++;
  }

  // Update button hold time variables
  button_left_pressed = digitalRead(pin_button_left);
  button_right_pressed = digitalRead(pin_button_right);
  if (button_left_pressed)
  {
    button_left_time++;
  }
  else
  {
    button_left_time = 0;
  }
  if (button_right_pressed)
  {
    button_right_time++;
  }
  else
  {
    button_right_time = 0;
  }

  // If in display mode '1',
  if (display_mode == 'l')
  {
    // If the right button has been pressed, switch to display mode 'd'
    if (button_right_time == 0 && button_right_time_previous >= 1 && button_right_time_previous <= 50)
    {
      display_mode = 'd';
      display_index = 0;
    }
    // If the left button has been pressed, switch to display mode 's'
    if (button_left_time == 0 && button_left_time_previous >= 1 && button_left_time_previous <= 50)
    {
      display_mode = 's';
      display_index = 0;
    }
  }

  // If in display mode 'd',
  else if (display_mode == 'd')
  {
    // If both buttons have been released after at least 3 seconds,
    if ((button_left_time == 0 || button_right_time == 0) && button_left_time_previous >= 300
        && button_right_time_previous >= 300)
    {
      beep();
      // If the display position is -3, write to EEPROM,
      if (display_index == -3)
      {
        Serial.print("Time: ");
        Serial.print(millis() / 1000.0);
        Serial.println(" | Starting Write to EEPROM");
        writeToEEPROM();
        Serial.print("Time: ");
        Serial.print(millis() / 1000.0);
        Serial.println(" | Ending Write to EPROM");
        loops_since_action = 0;
      }
      // If the display position is -2, read from EEPROM,
      else if (display_index == -2)
      {
        Serial.print("Time: ");
        Serial.print(millis() / 1000.0);
        Serial.println(" | Starting Read from EEPROM");
        readFromEEPROM();
        Serial.print("Time: ");
        Serial.print(millis() / 1000.0);
        Serial.println(" | Ending Read from EEPROM");
        loops_since_action = 0;
      }
    }
    // If both buttons have been released after at least 0.5 seconds,
    else if ((button_left_time == 0 || button_right_time == 0) && button_left_time_previous >= 50
             && button_right_time_previous >= 50)
    {
      beep();
      // If the display position is -1, read from SRAM
      if (display_index == -1)
      {
        Serial.print("Time: ");
        Serial.print(millis() / 1000.0);
        Serial.println(" | Starting Read from SRAM");
        readFromSRAM();
        Serial.print("Time: ");
        Serial.print(millis() / 1000.0);
        Serial.println(" | Ending Read from SRAM");
        loops_since_action = 0;
      }
    }
    // If the right button was pressed briefly, increase display index if under history length
    else if (button_right_time == 0 && button_right_time_previous >= 1 && button_right_time_previous <= 50)
    {
      if (display_index < recorded_temperatures_length)
      {
        display_index++;
      }
    }
    // If the left button was pressed briefly, decrease display index if greater than -3
    else if (button_left_time == 0 && button_left_time_previous >= 1 && button_left_time_previous <= 50)
    {
      if (display_index > -3)
      {
        display_index--;
      }
    }
    // If the right (but not left) button is held for 1 second, exit to display mode 'l'
    else if (button_right_time >= 100 && button_left_time == 0)
    {
      beep();
      display_mode = 'l';
    }
    // If the left (but not right) button was held for 1 second, jump to last data entry
    else if (button_left_time_previous >= 100 && button_right_time_previous == 0 && button_left_time == 0)
    {
      beep();
      if (current_record_index < recorded_temperatures_length)
      {
        display_index = current_record_index;
      }
      else
      {
        display_index = recorded_temperatures_length - 1;
      }
    }
  }
  // If in display mode 's',
  else if (display_mode == 's')
  {
    // If both buttons have been released after at least 0.5 seconds,
    if ((button_left_time == 0 || button_right_time == 0) && button_left_time_previous >= 50
        && button_right_time_previous >= 50)
    {
      beep();
      // If the display position is -3, reset LCD
      if (display_index == -3)
      {
        lcd.begin(16, 2);
        Serial.print("Time: ");
        Serial.print(millis() / 1000.0);
        Serial.println(" | LCD Reset");
      }
      // If the display position is -2, toggle sound
      if (display_index == -2)
      {

        Serial.print("Time: ");
        Serial.print(millis() / 1000.0);
        if (sound_enabled == 0)
        {
          sound_enabled = 1;
          Serial.println(" | Enabling Sound");
        }
        else
        {
          sound_enabled = 0;
          Serial.println(" | Disabling Sound");
        }
      }
      // If the display position is -1, toggle backlight
      if (display_index == -1)
      {

        Serial.print("Time: ");
        Serial.print(millis() / 1000.0);
        if (lcd_backlight_enabled == 0)
        {
          lcd_backlight_enabled = 1;
          Serial.println(" | Enabling Backlight");
        }
        else
        {
          lcd_backlight_enabled = 0;
          Serial.println(" | Disabling Backlight");
        }
      }
    }
    // If the right button was pressed briefly, increase display index if less than 0
    else if (button_right_time == 0 && button_right_time_previous >= 1 && button_right_time_previous <= 50)
    {
      if (display_index < 0)
      {
        display_index++;
      }
    }
    // If the left button was pressed briefly, decrease display index if greater than -3
    else if (button_left_time == 0 && button_left_time_previous >= 1 && button_left_time_previous <= 50)
    {
      if (display_index > -3)
      {
        display_index--;
      }
    }
    // If the left (but not right) button is held for 1 second, exit to display mode 'l'
    else if (button_left_time >= 100 && button_right_time == 0)
    {
      beep();
      display_mode = 'l';
    }
  }

  // Save the button press times from this cycle
  button_left_time_previous = button_left_time;
  button_right_time_previous = button_right_time;

  // Every 30 seconds, update and print to serial port the supply voltage
  if (seconds_elapsed % 30 == 0 && is_first_loop_of_second == 1)
  {
    updateSupplyVoltage();
    Serial.print("Time: ");
    Serial.print(millis() / 1000.0);
    Serial.print(" | ");
    Serial.print("Arduino Power Supply: ");
    Serial.print(supply_voltage);
    Serial.println(" mV");
  }

  // Every 75 cycles, read, update, and print the temperature, and turn on LED 1
  if (loop_number % 75 == 0)
  {
    Serial.print("Time: ");
    Serial.print(millis() / 1000.0);
    Serial.print(" | ");
    Serial.print("Current Temperature: ");
    Serial.print(updateTemperature());
    Serial.print(" | Median Temperature: ");
    Serial.println(recent_temperatures_median);
    led_1_enabled = 1;
  }

  // Every 75 cycles, delayed by 20 cycles, turn off LED 1
  if ((loop_number - 20) % 75 == 0)
  {
    led_1_enabled = 0;
  }

  // Except in the first second,
  if (seconds_elapsed != 0)
  {
    // Every sampling_period seconds, record the current median temperature and turn on LED 2
    if (seconds_elapsed % sampling_period == 0 && is_first_loop_of_second == 1)
    {
      led_2_enabled = 1;
      Serial.print("Time: ");
      Serial.print(millis() / 1000.0);
      Serial.print(" | ");
      Serial.print("Sample ");
      Serial.print(current_record_index);
      // If the record array is not full,
      if (current_record_index < recorded_temperatures_length)
      {
        recorded_temperatures[current_record_index] = round(recent_temperatures_median);
        Serial.print(": ");
        Serial.println(recorded_temperatures[current_record_index]);
      }
      // If the record array is full,
      else
      {
        Serial.println(" Not Saved; Memory Full");
      }
      current_record_index++;
    }

    // Every (sampling_period) seconds, delayed by 1 second, turn off LED 2
    if ((seconds_elapsed - 1) % (sampling_period) == 0 && is_first_loop_of_second == 1)
    {
      // If the record array is not full,
      if (current_record_index < recorded_temperatures_length)
      {
        led_2_enabled = 0;
      }
    }
  }

  // Beep on button press
  if (button_left_time == 1 || button_right_time == 1)
  {
    beep();
  }

  // Every 5 cycles, update the screen
  if (loop_number % 5 == 0)
  {
    updateScreen();
  }

  // Update the digital outputs:
  updateDigitalOutputs();

  // Delay 10 ms before starting next cycle
  delay(10);
}

float updateTemperature()
{
  float current_temperature;
  
  // Calculate current temperature
  float voltage = (analogRead(pin_temp_sensor) / 1024.0) * supply_voltage / 1000.0;
  current_temperature = (voltage - 0.5) * 100.0 * 9.0 / 5.0 + 32.0;

  // Add to recent_temperatures after shifting old data while counting data points
  int recent_temperaturesNumber = 0;
  for (int i = recent_temperatures_length - 2; i >= 0; i--)
  {
    if (recent_temperatures[i])
    {
      recent_temperaturesNumber++;
      recent_temperatures[i + 1] = recent_temperatures[i];
    }
  }
  recent_temperaturesNumber++;
  recent_temperatures[0] = current_temperature;
  recent_temperatures_median = median(recent_temperatures, recent_temperaturesNumber);

  return current_temperature;
}

void updateScreen()
{
  // If in display mode 'l', print the time elapsed and median temperature
  if (display_mode == 'l')
  {
    lcd.setCursor(0, 0);
    lcd.print(hours_elapsed);
    lcd.print(":");
    if ((minutes_elapsed % 60) < 10)
    {
      lcd.print("0");
    }
    lcd.print(minutes_elapsed % 60);
    lcd.print(":");
    if ((seconds_elapsed % 60) < 10)
    {
      lcd.print("0");
    }
    lcd.print(seconds_elapsed % 60);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(round(recent_temperatures_median));
    lcd.print((char)223);
    lcd.print("F");
    lcd.print("                ");
  }

  // If in display mode 'd',
  if (display_mode == 'd')
  {
    // If the right button is being held, prompt to keep holding
    if (button_right_time >= 20 && button_left_time == 0)
    {
      lcd.setCursor(0, 0);
      lcd.print("Exit Data Mode");
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print("Continue Holding");
      lcd.print("                ");
    }
    // If the left button is being held, prompt to keep holding
    else if (button_left_time >= 20 && button_right_time == 0)
    {
      lcd.setCursor(0, 0);
      lcd.print("Jump to Last");
      lcd.print("                ");
      lcd.setCursor(0, 1);
      // If the button has been held for more than 1 s, prompt to release
      if (button_left_time >= 100)
      {
        lcd.print("Release");
      }
      // Otherwise, prompt to keep holding
      else
      {
        lcd.print("Continue Holding");
      }
      lcd.print("                ");
    }
    // In position -3, show Write to EEPROM option
    else if (display_index == -3)
    {
      lcd.setCursor(0, 0);
      lcd.print("Write to EEPROM");
      lcd.print("                ");
      lcd.setCursor(0, 1);
      // If the buttons have been held for more than 3 s, prompt to release
      if (button_left_time >= 300 && button_right_time >= 300)
      {
        lcd.print("Release");
      }
      // If the buttons are being held, prompt to keep holding
      else if (button_left_time >= 1 && button_right_time >= 1)
      {
        lcd.print("Continue Holding");
      }
      // If the action just completed, show confirmation
      else if (loops_since_action < 100)
      {
        lcd.print("Write Complete");
      }
      // If the buttons are not being held, print instruction to hold it
      else
      {
        lcd.print("Hold L and R");
      }
      lcd.print("                ");
    }
    // In position -2, show Read from EEPROM option
    else if (display_index == -2)
    {
      lcd.setCursor(0, 0);
      lcd.print("Read from EEPROM");
      lcd.print("                ");
      lcd.setCursor(0, 1);
      // If there is a Serial connection,
      if (Serial)
      {
        // If the buttons have been held for more than 3 s, prompt to release
        if (button_left_time >= 300 && button_right_time >= 300)
        {
          lcd.print("Release");
        }
        // If the buttons are being held, prompt to keep holding
        else if (button_left_time >= 1 && button_right_time >= 1)
        {
          lcd.print("Continue Holding");
        }
        // If the action just completed, show confirmation
        else if (loops_since_action < 100)
        {
          lcd.print("Read Complete");
        }
        // If the buttons are not being held, print instruction to hold it
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
    // In position -1, show Read from SRAM option
    else if (display_index == -1)
    {
      lcd.setCursor(0, 0);
      lcd.print("Read from SRAM");
      lcd.print("                ");
      lcd.setCursor(0, 1);
      // If there is a Serial connection,
      if (Serial)
      {
        // If the buttons have been held for more than 0.5 s, prompt to release
        if (button_left_time >= 50 && button_right_time >= 50)
        {
          lcd.print("Release");
        }
        // If the buttons are being held, prompt to keep holding
        else if (button_left_time >= 1 && button_right_time >= 1)
        {
          lcd.print("Continue Holding");
        }
        // If the action just completed, show confirmation
        else if (loops_since_action < 100)
        {
          lcd.print("Read Complete");
        }
        // If the buttons are not being held, print instruction to hold it
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
    // In position 0, show "Data View" title and sampling period
    else if (display_index == 0)
    {
      lcd.setCursor(0, 0);
      lcd.print("Data View");
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print("Samp. Per.: ");
      lcd.print(sampling_period);
      lcd.print("                ");
    }
    // In positions greater than 0, show corresponding sample, if any
    else if (display_index > 0)
    {
      lcd.setCursor(0, 0);
      lcd.print("Sample: ");
      lcd.print(display_index - 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      if (recorded_temperatures[display_index - 1])
      {
        lcd.print("Temp: ");
        lcd.print(recorded_temperatures[display_index - 1]);
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
  // If in display mode 's',
  if (display_mode == 's')
  {
    // If the left button is being held, prompt to keep holding
    if (button_left_time >= 20 && button_right_time == 0)
    {
      lcd.setCursor(0, 0);
      lcd.print("Exit Settings");
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print("Continue Holding");
      lcd.print("                ");
    }
    // In position -3, show LCD reset option
    else if (display_index == -3)
    {
      lcd.setCursor(0, 0);
      lcd.print("LCD Reset");
      lcd.print("                ");
      lcd.setCursor(0, 1);
      // If the buttons have been held for more than 0.5 s, prompt to release
      if (button_left_time >= 50 && button_right_time >= 50)
      {
        lcd.print("Release");
      }
      // If the buttons are being held, prompt to keep holding
      else if (button_left_time >= 1 && button_right_time >= 1)
      {
        lcd.print("Continue Holding");
      }
      // If the buttons are not being held, print instruction to hold it
      else
      {
        lcd.print("Hold L and R");
      }
      lcd.print("                ");
    }
    // In position -2, show sound toggle option
    else if (display_index == -2)
    {
      lcd.setCursor(0, 0);
      lcd.print("Sound Toggle");
      lcd.print("                ");
      lcd.setCursor(0, 1);
      // If the buttons have been held for more than 0.5 s, prompt to release
      if (button_left_time >= 50 && button_right_time >= 50)
      {
        lcd.print("Release");
      }
      // If the buttons are being held, prompt to keep holding
      else if (button_left_time >= 1 && button_right_time >= 1)
      {
        lcd.print("Continue Holding");
      }
      // If the buttons are not being held, print instruction to hold it
      else
      {
        lcd.print("Hold L and R");
      }
      lcd.print("                ");
    }
    // In position -1, show backlight toggle option
    else if (display_index == -1)
    {
      lcd.setCursor(0, 0);
      lcd.print("Backlight Toggle");
      lcd.print("                ");
      lcd.setCursor(0, 1);
      // If the buttons have been held for more than 0.5 s, prompt to release
      if (button_left_time >= 50 && button_right_time >= 50)
      {
        lcd.print("Release");
      }
      // If the buttons are being held, prompt to keep holding
      else if (button_left_time >= 1 && button_right_time >= 1)
      {
        lcd.print("Continue Holding");
      }
      // If the buttons are not being held, print instruction to hold it
      else
      {
        lcd.print("Hold L and R");
      }
      lcd.print("                ");
    }
    // In position 0, show "Settings" title
    else if (display_index == 0)
    {
      lcd.setCursor(0, 0);
      lcd.print("Settings");
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print("Free SRAM: ");
      lcd.print(freeRAM());
      lcd.print("                ");
    }
  }
  return;
}

// Update the digital outputs based on corresponding variables
void updateDigitalOutputs()
{
  if (led_1_enabled == 1)
  {
    digitalWrite(pin_led_1, HIGH);
  }
  else
  {
    digitalWrite(pin_led_1, LOW);
  }

  if (led_2_enabled == 1)
  {
    digitalWrite(pin_led_2, HIGH);
  }
  else
  {
    digitalWrite(pin_led_2, LOW);
  }

  if (lcd_backlight_enabled == 1)
  {
    digitalWrite(pin_lcd_bl1, HIGH);
  }
  else
  {
    digitalWrite(pin_lcd_bl1, LOW);
  }
  return;
}

// Update the power supply voltage for proper thermometer calibration
void updateSupplyVoltage()
{
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
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high << 8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000

  supply_voltage = result; // Vcc in millivolts
  return;
}

// Print to the serial port each record in the SRAM
void readFromSRAM()
{
  for (int i = 0; i < current_record_index; i++)
  {
    if (i < recorded_temperatures_length)
    {
      Serial.print(i);
      Serial.print(",");
      Serial.println(recorded_temperatures[i]);
    }
    else
    {
      break;
    }
  }
}

// Print to the serial port each record under 200 in the EEPROM
void readFromEEPROM()
{
  byte value;
  int i = 0;
  while (i <= E2END)
  {
    value = EEPROM.read(i);
    if (value < 200)
    {
      Serial.print(i);
      Serial.print(",");
      Serial.println(value, DEC);
    }
    else
    {
      break;
    }
    i++;
  }
}

// Write each record in SRAM to the EEPROM
void writeToEEPROM()
{
  int i;
  for (i = 0; i < current_record_index; i++)
  {
    if (i < E2END && i < recorded_temperatures_length) // E2END is the maximum EEPROM address
    {
      EEPROM.write(i, recorded_temperatures[i]);
    }
    else
    {
      break;
    }
  }
  EEPROM.write(i, 255); // 255 indicates end of actual data
}

// Find the median of an array of n float variables
float median(float input[], int n)
{
  float temp;
  int i, j;
  float x[n];
  for (i = 0; i < n; i++)
  {
    x[i] = input[i];
  }
  // Sort the data
  for (i = 0; i < n - 1; i++)
  {
    for (j = i + 1; j < n; j++)
    {
      if (x[j] < x[i])
      {
        temp = x[i];
        x[i] = x[j];
        x[j] = temp;
      }
    }
  }
  // If n is even, return the mean of the two middle elements
  if (n % 2 == 0)
  {
    return ((x[n / 2] + x[n / 2 - 1]) / 2.0);
  }
  // Otherwise, return the middle element
  else
  {
    return x[n / 2];
  }
}

// If sound is enabled, play a beep
void beep()
{
  if (sound_enabled == 1)
  {
    tone(pin_piezo, 3000, 50);
  }
}

// Return the available memory between the heap and the stack
int freeRAM()
{
  // from https://learn.adafruit.com/memories-of-an-arduino/measuring-free-memory
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

