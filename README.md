# Arduino Temperature Logger
This Arduino project constantly samples the temperature, records the 
median of recent samples at a defined frequency, takes user input from 
two  buttons, and outputs to a 16-by-2 liquid crystal display (LCD), 
the serial monitor, two LEDs, and a piezo buzzer.

![Project Photo](./project_photo.jpg)

## Temperature Measuring and Recording
The temperature is measured approximately once per second by a TMP36 
Temperature Sensor. A formula which considers the Arduino supply voltage 
is used to convert the analog input from the sensor to a temperature in 
degrees Fahrenheit. 

Due to variations in the Arduino power supply and the precision of the 
analog input measurements, the measurements include noise that can 
cause each measurement to vary up to several degrees from the previous 
one. To reduce the effect of the noise, each measurement is stored as a 
float value in an array of the 30 most recent measurements. After each 
measurement, the median of the array (excluding any unfilled values) is 
calculated. The median is then printed to the serial monitor and the 
LCD, and LED 1 glows for about 0.2 seconds to indicate that a 
measurement has occurred.

In addition to an array of the 30 most recent measurements, the program 
stores 500 values in a history array as bytes stored in the static 
random access memory (SRAM). The most recently measured median 
temperature is added to this array at a defined sampling period, and 
LED 2 glows for one second. The sampling period is by default set to 4 
minutes, allowing approximately 33.3 hours of samples before the history 
is full. Once the history array fills, LED 2 glows continuously to alert 
the user.

## User Interface
The main screen of the LCD shows the time elapsed since the program 
started and the most recently calculated median temperature. A brief 
press of the left button enters the settings menu, and a brief press 
of the right button enters the data view menu.

### Settings Menu
Anywhere in the settings menu, pressing and holding the left button 
for approximately one second will exit the settings menu and return to 
the main screen. Brief presses of the left and right buttons move 
between the screens of the settings menu.

The first screen shows the settings menu title as well as the current 
free static random access memory (SRAM) available in the Arduino for 
debugging purposes.

Pressing the left button shows the **Backlight Toggle** option. 
Pressing and holding both buttons for approximately one second either 
disables or enables the LCD backlight.

The next screen to the left is the **Sound Toggle** option. When sound 
is enabled, the piezo buzzer plays a 3-kHz beep whenever a button is 
pressed or the holding of a button or buttons is complete.

The final screen is the **LCD Reset** option, which re-initializes the 
LCD. This is necessary for the LCD to work properly in some cases 
after a connection to the LCD is broken and reconnected.

### Data View Menu
Anywhere in the data view menu, pressing and holding the right button 
for approximately one second will exit the data view menu and return 
to the main screen. Brief presses of the left and right buttons move 
between the screens of the menu.

The main screen shows the menu title and the sampling period in seconds.

Moving to the right of the main screen shows the values stored in the 
history array. "No Data" is shown for samples that have not yet been 
recorded. Pressing and holding the left button for approximately one 
second will jump to the last recorded sample in the array.

To the left of the main screen are read/write actions which can be 
activated by pressing and holding the left and right buttons.

**Read to SRAM** prints to the serial monitor on the computer, if 
connected, the sample number and value, separated by a comma, of each 
sample stored in the history array.

When the Arduino loses power or resets, all data in the SRAM is lost. 
Fortunately, the Arduino also has electrically erasable programmable 
read-only memory (EEPROM), in which data stored is not lost on resets. 
Because the EEPROM has a limited number of read/write 
s, it would 
not be practical to record every sample in the EEPROM. However, for 
cases where it is necessary to reset the Arduino after recording 
samples but before transferring the data to the serial monitor (e.g., 
when the Arduino is being powered by a battery instead of a computer 
during data collection), the program includes options to write to and 
read from the EEPROM. **Write to EEPROM** saves the current history 
array to the EEPROM. **Read from EEPROM** prints to the serial monitor 
on the computer, if connected, the sample number and value, separated 
by a comma, of each sample stored in the EEPROM.