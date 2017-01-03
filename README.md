# Arduino Temperature Logger
This Arduino project constantly samples the temperature, records the 
median of recent samples at a defined frequency, takes user input from 
two  buttons, and outputs to an LCD, the serial monitor, two LEDs, and 
a piezo buzzer.

## Program Structure
The program 
* reads and responds to digital inputs every cycle, 
* reads and temporarily records the temperature every 75 cycles, 
* records the median of the most recent 30 measurements every 4 
  minutes,
* updates the supply voltage every 30 seconds, and
* updates the LCD every 5 cycles.

Each cycle is separated by a delay of 10 milliseconds.

## Temperature Measuring
The temperature is measured by a TMP36 Temperature Sensor. A formula 
which considers the Arduino supply voltage is used to convert the 
analog input from the sensor to a temperature in degrees Fahrenheit. 

Due to variations in the Arduino power supply and the precision of the 
analog input measurements, the measurements include noise that can 
cause each measurement to vary up to several degrees from the previous 
one. To reduce the effect of the noise, each measurement is stored as a 
float value in an array of the 30 most recent measurements. After each 
measurement, the median of the array (excluding any unfilled values) is 
calculated. The median is then printed to the serial monitor and the 
LCD, and LED 1 glows for 20 cycles to indicate that a measurement has 
occurred.

## Temperature Recording
In addition to an array of the 30 most recent measurements, the program 
stores 500 byte values in a history array. The most recently measured 
median temperature is added to this array every `samplingPeriod` 
seconds, and LED 2 glows for one second. The sampling period is by 
default set to 4 minutes, allowing approximately 33.3 hours of samples 
before the history is full. Once the history array fills, LED 2 glows 
continuously to alert the user.

