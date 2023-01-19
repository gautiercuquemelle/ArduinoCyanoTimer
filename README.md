# ArduinoCyanoTimer
Arduino code for a simple timer controlling a relay

Used Libraries :
- LiquidCrystal_I2C : https://github.com/johnrickman/LiquidCrystal_I2C

Component list :
- 1 x Arduino Uno R3
- 1 x 16x2 LCD display with I2C
- 1 x Double realay module
- 1 x Temperature and humidity sensor module
- 6 x Resistor 10kOhm
- 4 x push buttons
- 2 x capacitor 10 nF
- 1 x prototype board
- wires, box, soldering iron, patience and good music

Schema :
- will come soon

FAQ :
Q : Why didn't use the timer_0, timer_1 and timer_2 instead of reiventing wheel ?
R : Less code for same result. I don't need precision lower than 1 second, so it's OK

Q : I don't have a temperature sensor module and don't want to use it
R : Connect the digital pin 7 of the Arduino to GND. It will disable the functionality in code
