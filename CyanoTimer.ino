#include "LiquidCrystal_I2C.h" // Used to pilot I2C controler of the LCD screen
#include "EEPROM.h" // Used to memorize timer value after shut off

// Using 0x27 as address value, check specifications of yours
LiquidCrystal_I2C LCD(0x27,16,2); // defining LCD screen type to 16 x 2

// Constants used for digital input
const byte BtnPlusPin = 4; 
const byte BtnMinusPin = 5;
const byte BtnStartPin = 2;
const byte BtnResetPin = 3;

// If this pin is connected to +VCC, the program will not try to read temperature sensor values or control the relay for fan control
const byte DisableTempPin = 7; 

// Constants used for digital outputs
const byte CmdRelayPin = 9;
const byte CmdFanPin = 10;

// Constante utilisée pour l'ES du capteur de température
const byte SensorTempPin = 8; 

// Liste des états du minuteur
const byte TimerState_Stopped = 0;
const byte TimerState_Running = 1;
const byte TimerState_Paused = 2;

// Variables used to manage the timer
volatile byte timerValueFromMemory = 0; // Value stored in EEPROM memory (in minutes)
volatile long timerValueFromInput = 5; // user input value (in minutes)
volatile long timerValueFromInputInMilliSeconds = 0; // Calculated from the input value
volatile long elapsedTimeInMilliseconds = 0; // Current elapsed time, calculated in loop() function
volatile long referenceTimeAtResume = 0; // used to manage the pause / resume function
volatile byte timerState = TimerState_Stopped; // Stopped by default at the starting of the program

// Variables used to manage temperature module
volatile byte useTemperatureModule = 0; // Will store value read on the SensorTempPin
volatile float measuredTemperature; // Measured temperatured read from the temperature module
volatile int cptCycles = 0; // Used to do actions only each N cycles (ex : reading temperature)

// Used to store the 5 values sent by temperature module : 
// Humidity (integer part), Humidity (decimal part), Temperature (integrer part), Temperature (decimal part), checksum
byte temperatureModuleData[5]; 


// Special characters design (Stop / Play / Pause)
const byte charStop = 0;
const byte charStopDefinition[8] = {
0b00000,
0b00000,
0b11111,
0b11111,
0b11111,
0b11111,
0b11111,
0b00000};

const byte charPause = 1;
const byte charPauseDefinition[8] = {
0b00000,
0b00000,
0b11011,
0b11011,
0b11011,
0b11011,
0b11011,
0b00000};

const byte charPlay = 2;
const byte charPlayDefinition[8] = {
0b00000,
0b10000,
0b11000,
0b11100,
0b11110,
0b11100,
0b11000,
0b10000};

const byte charDegree = 3;
const byte charDegreeDefinition[8] = {
0b00100,
0b01010,
0b00100,
0b00000,
0b00000,
0b00000,
0b00000,
0b00000};

// Function called when the program start
void setup()
{
  // initializing LCD display 
  LCD.init(); 
  LCD.backlight();
  LCD.clear();

  // Registering in LCD module  the special chars
  LCD.createChar(charStop, charStopDefinition); 
  LCD.createChar(charPause, charPauseDefinition); 
  LCD.createChar(charPlay, charPlayDefinition); 
  LCD.createChar(charDegree, charDegreeDefinition);
  
  // Initialising serial port for debug purposes
  Serial.begin(9600);
  
  // Initializing digital inputs
  pinMode(BtnStartPin, INPUT); 
  pinMode(BtnPlusPin, INPUT);
  pinMode(BtnMinusPin, INPUT);
  pinMode(BtnResetPin, INPUT);
  pinMode(DisableTempPin, INPUT);

  // Initializing digital outputs
  pinMode(CmdRelayPin, OUTPUT);  
  pinMode(CmdFanPin, OUTPUT);  
  pinMode(SensorTempPin, OUTPUT);

  // Opening the relays
  Serial.println("Opening relays");
  digitalWrite(CmdRelayPin, HIGH);
  digitalWrite(CmdFanPin, HIGH);
     
  // Initializing interrupts : only 2 available on Arduino Uno :-(
  attachInterrupt(digitalPinToInterrupt(BtnStartPin), onButtonStartPressed, RISING); 
  attachInterrupt(digitalPinToInterrupt(BtnResetPin), onButtonResetPressed, RISING);   

  // Check if temperature modeul will be used
  useTemperatureModule = digitalRead(DisableTempPin) ^ 1; // Inversion of the value read from the DisableTempPin  
  Serial.print("using temerature module : ");
  Serial.println(useTemperatureModule == 1 ? "Yes" : "No");

  // Reading memorized value for the timer from EEPROM memory
  timerValueFromMemory = EEPROM.read(0); // Reading the byte, enough to store the timer value
  Serial.print("Read memorized value of timer from EEPROM : ");
  Serial.print(timerValueFromMemory);
  if(timerValueFromMemory > 0 && timerValueFromMemory < 100)
  {
    timerValueFromInput = timerValueFromMemory;
    Serial.println(" - OK, will use this value");
  }
  else
  {
    Serial.println(" - Invalid value, keeping the default one");
  }

  // First reading of the temperature (if we use the temperature module)
  ManageTemperatureModule();
}

// Main loop function
void loop()
{
  ManageTimer(); // Manage button inputs and timer calculations
  
  // Measuring temperature every 50 cycles (~ every second)
  if(cptCycles++ >=50)
  {
    ManageTemperatureModule(); // Read value from temperature module
	  cptCycles = 0;
  }

   ManageDisplay(); // Display informations and status on LCD display

  delay(100); // wait 100 ms
}

// Function called in the main loop to manage events and controls related to the timer operation
void ManageTimer()
{
  // Not enough interruptions to manage the 4 buttons, we just check if the "Plus" and "Minus" buttons are pressed
  // Only takes into account if the counter status is "Off".
  if(timerState == TimerState_Stopped )
  {
    if(digitalRead(BtnPlusPin) == HIGH && timerValueFromInput < 99)
    {
      // Add one minute
      timerValueFromInput += 1;      
    }
    
	  if(digitalRead(BtnMinusPin) == HIGH && timerValueFromInput > 0)
    {
      // Remove one minute
      timerValueFromInput-=1;
    }
  }

  // If timer is running we calculate and controle elapsed time
  if(timerState == TimerState_Running)
  {
    elapsedTimeInMilliseconds = millis() - referenceTimeAtResume;
	
    // If elapsed time is greated than input value, the we stop and reset the timer
    if(timerValueFromInputInMilliSeconds <= elapsedTimeInMilliseconds)
    {
      MamageTimerEnd();
    }
  }  
}

// Function called in the main loop to manage events and controls related to temperature control
void ManageTemperatureModule()
{
  if(useTemperatureModule == 0)
  {
    return;
  }
  
  measuredTemperature = ReadTemperature();
  
  if(measuredTemperature > 30) // Can be defined as you will
  {
    StartVenting();
  }
  else
  {
    StopVenting();
  }
}

// Function called in the main loop to control the display of information on the LCD display
void ManageDisplay()
{
  long remainingSeconds = (timerValueFromInputInMilliSeconds - elapsedTimeInMilliseconds) / 1000;
  int displayedMinutes = remainingSeconds / 60;
  int displayedSeconds = remainingSeconds - displayedMinutes * 60;
  
  LCD.setCursor(0, 0);
  LCD.print("Delay: "); 
  LCD.print(timerValueFromInput); 
  LCD.print(" min. "); 
      
  switch(timerState){
    case TimerState_Stopped :
      LCD.setCursor(0, 1);
      LCD.write((byte)charStop);
      LCD.setCursor(3, 1);
      LCD.print("     ");
      break;
    
    case TimerState_Running :
      LCD.setCursor(0, 1);
      LCD.write((byte)charPlay);
      
      LCD.setCursor(3, 1);
      LCD.print(displayedMinutes);
      LCD.print("\""); 
      LCD.print(displayedSeconds);
      //LCD.print("s  "); 
      break;
    
    case TimerState_Paused :
      LCD.setCursor(0, 1);
      LCD.write((byte)charPause);
      break;    
  }

  if(useTemperatureModule == 1)
  {
    LCD.setCursor(11, 1);
        
    LCD.print(round(measuredTemperature));
    LCD.write((byte)charDegree);
    LCD.print("C");
  }
}

void TurnOnLamp(){
	Serial.println("Closing lamp relay");
	digitalWrite(CmdRelayPin, LOW);
}

void TurnOffLamp(){
	Serial.println("Opening lamp relay");
	digitalWrite(CmdRelayPin, HIGH);
}

void StartVenting()
{
	Serial.println("Closing fan relay");
  digitalWrite(CmdFanPin, LOW);
}

void StopVenting()
{
	Serial.println("Opening fan relay");
	digitalWrite(CmdFanPin, HIGH);
}

void MamageTimerEnd()
{
  timerState = TimerState_Stopped;
  elapsedTimeInMilliseconds = 0;
  TurnOffLamp();
}

// Called when button "Start" is pressed
void onButtonStartPressed() 
{
  if(timerValueFromInput == 0)
  {
    // Can't start if value of timer is 0
    return;
  }

  // If timer is not already running
  if(timerState != TimerState_Running)
  {
    // And was at the "stopped" status
    if(timerState == TimerState_Stopped)
	  {
      // Write input value in EEPROM only if different from current memorized value
      // EEPROM has a limited number of read/write values (~10000), let's be thrifty !
      if(timerValueFromInput != timerValueFromMemory)
      {
        Serial.print("Write input value of timer in EEPROM : ");
        Serial.print(timerValueFromInput);
        EEPROM.write(0, timerValueFromInput);

        timerValueFromMemory = timerValueFromInput; // to avoid rewriting in memory at next starts
      }

      // converting input value in minutes to milliseconds
      timerValueFromInputInMilliSeconds = timerValueFromInput * 60 * 1000;

      // the reference time will be equal to the number of millis ellapsed since the starting of the program
      referenceTimeAtResume = millis();
    }
    else // Current state is "Paused"
    {
      // Calculate a new reference time
      referenceTimeAtResume = millis() - elapsedTimeInMilliseconds;
    }

    // Change the status to "Running"
    timerState = TimerState_Running;

    // Turn on the lamp
    TurnOnLamp();    
  }
  else // Current status in "Running"
  {
    // Change the status to "Paused"
    timerState = TimerState_Paused;

    // Turn off the lamp
    TurnOffLamp();  
  }
}

// Called when button "Reset" is pressed (turn off lamp + reset timer)
void onButtonResetPressed() 
{
  MamageTimerEnd();
}

// All code behind has been copy/pasted from here : https://arduinomodules.info/ky-015-temperature-humidity-sensor-module/
float ReadTemperature()
{
  start_test();
  Serial.print("Humdity = ");
  Serial.print(temperatureModuleData[0], DEC); //Displays the integer bits of humidity;
  Serial.print('.');
  Serial.print(temperatureModuleData[1], DEC); //Displays the decimal places of the humidity;
  Serial.print("% - ");
  Serial.print("Temperature = ");
  Serial.print(temperatureModuleData[2], DEC); //Displays the integer bits of temperature;
  Serial.print('.');
  Serial.print(temperatureModuleData[3], DEC); //Displays the decimal places of the temperature;
  Serial.print("C ");
  byte checksum = temperatureModuleData[0] + temperatureModuleData[1] + temperatureModuleData[2] + temperatureModuleData[3];
  if (temperatureModuleData[4] != checksum) 
    Serial.println("-- Checksum Error!");
  else
    Serial.println("-- Checksum OK");

  float measuredTemperature = temperatureModuleData[2];
  float decimalPart = temperatureModuleData[3];
  
  // Converting integer + decimal parts into numeric value
  measuredTemperature += decimalPart / 10; // (read precision is 1/10 of degree)

  return measuredTemperature;
}

void start_test()
{
  digitalWrite(SensorTempPin, LOW); //Pull down the bus to send the start signal
  delay(30); //The delay is greater than 18 ms so that DHT 11 can detect the start signal
  digitalWrite(SensorTempPin, HIGH);
  delayMicroseconds(40); //Wait for DHT11 to respond
  pinMode(SensorTempPin, INPUT);
  while(digitalRead(SensorTempPin) == HIGH);
  delayMicroseconds(80); //The DHT11 responds by pulling the bus low for 80us;
  
  if(digitalRead(SensorTempPin) == LOW)
    delayMicroseconds(80); //DHT11 pulled up after the bus 80us to start sending temperatureModuleDataa;

  for(int i = 0; i < 5; i++) //Receiving temperature and humidity temperatureModuleDataa, check bits are not considered;
    temperatureModuleData[i] = read_temperatureModuleDataa();
	
  pinMode(SensorTempPin, OUTPUT);
  digitalWrite(SensorTempPin, HIGH); //After the completion of a release of temperatureModuleDataa bus, waiting for the host to start the next signal
}

byte read_temperatureModuleDataa()
{
  byte i = 0;
  byte result = 0;
  for (i = 0; i < 8; i++) 
  {
    while (digitalRead(SensorTempPin) == LOW); // wait 50us
    delayMicroseconds(30); //The duration of the high level is judged to determine whether the temperatureModuleDataa is '0' or '1'
    if (digitalRead(SensorTempPin) == HIGH)
      result |= (1 << (8 - i)); //High in the former, low in the post
  
    while (digitalRead(SensorTempPin) == HIGH); //temperatureModuleDataa '1', waiting for the next bit of reception
  }
  return result;
}
