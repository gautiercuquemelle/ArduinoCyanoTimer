# ArduinoCyanoTimer
## Arduino code for a simple timer controlling 2 relays : 
- #1 to control anything you want (I use it to control an UV lamp used for cyanotype process). Take care of the voltage and current ! Relay modules used can handle 220V AC and 10A or 30V DC and 10A. I won't take any responsibility of any damage if you plug a 2500W heater (or for any other bad idea)
- #2 to control a venting system used to cool the lamp if temperature is greater than 30°C

## Used Libraries :
- LiquidCrystal_I2C : https://github.com/johnrickman/LiquidCrystal_I2C

## Component list :
- 1 x Arduino Uno R3
- 1 x 16x2 LCD display with I2C (I used one from iHaospace with PCF8574T chipset)
- 1 x Double realay module (Yizhet 5V relay module with Songle SRD-05VDC relays)
- 1 x Temperature and humidity sensor module (KY-015 DHT)
- 4 x Resistor 10kOhm
- 4 x push buttons
- 4 x capacitor 10 nF
- 1 x prototype board
- wires, box, soldering iron, patience and good music

## Schema :
Available here : https://wokwi.com/projects/354364770938972161<br>
- It is a simplified version (no capacitor) : capacitors are used to limit bounce effect on push buttons, and are placed between the +VCC and the digital pin
- The code is not guaranteed to be up to date (use code in github to be sure)
- You can add LEDs to have a visual feedback of the relay commands. Simply connect a resistor and a LED between to the output pin and the +VCC 

## FAQ<br><br>
<b>Q</b> : I have a brand new Arduino Uno, all required components have been properly soldered (or plugged on breadboard) but I don't know what to do with the .ino file<br>
<b>A</b> : Install Arduino IDE on your computer (Windows / Mac / Linux) from here : https://www.arduino.cc/en/software then follow starting guide in file HowToUploadProgramToArduino.pdf<br>

<b>Q</b> : I have uploaded program in my Arduino but screen stays blank (or blue)<br>
<b>A1</b> : On the I2C module there is a potentiometer to adjust contrast : try to adjust contrast, it can be as easy<br>
<b>A2</b> : Check the chipset reference on the I2C module of the LCD screen. For 8574T use 0x27, for 8574AT use 0x3F, for other ref check in the spec. Value must be changed in the code : check the first line after the "#Include" lines<br>

<b>Q</b> : Why didn't use the timer_0, timer_1 and timer_2 instead of reiventing wheel ?<br>
<b>A</b> : Less code for same result. I don't need precision lower than 1 second, so it's OK<br>

<b>Q</b> : I don't have a temperature sensor module and don't want to use it<br>
<b>A</b> : Connect the digital pin 7 of the Arduino to +VCC. It will disable the functionality in code (measure and display)<br>
