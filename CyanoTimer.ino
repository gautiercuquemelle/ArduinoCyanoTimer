#include "Wire.h"
#include "LiquidCrystal_I2C.h"

LiquidCrystal_I2C LCD(0x27,16,2); // définit le type d'écran lcd 16 x 2

// Constantes utilisées pour les entrées digitales
const byte BtnPlusPin = 4;
const byte BtnMoinsPin = 5;
const byte BtnStartPin = 2;
const byte BtnResetPin = 3;
const byte ShuntTemperature = 7;

// Constantes utilisées pour les sorties digitales
const byte CmdRelayPin = 9;
const byte CmdFanPin = 10;

// Constante utilisée pour l'ES du capteur de température
const byte capteurTempPin = 8; 

// Liste des états du minuteur
const byte listeEtatsMinuteur_Arret = 0;
const byte listeEtatsMinuteur_Marche = 1;
const byte listeEtatsMinuteur_Pause = 2;

// Variables utilisées pour le fonctionnement du minuteur
volatile long dureeSaisieEnMinutes = 5;
volatile long dureeSaisieEnMilliSecondes = 0; // Calculé à partir de la durée saisie en minutes
volatile long dureeEcouleeEnMillisecondes = 0;
volatile long valeurMillisDemarrageMinuteur = 0; // vaut "millis()" Au démarrage du minuteur, "millis()-dureeEcouleeEnMillisecondes" à la reprise après pause
volatile byte etatMinuteur = listeEtatsMinuteur_Arret; // Etat du minuteur à l'arret au démarrage

volatile byte MesureTemperatureActive = 0;
volatile float temperatureMesuree;
volatile int cptCycles = 0; // Utilisé pour effectuer certaines actions tous les N cycles (ex : lecture température)

const bool debugMode = true; // Active l'envoi des logs sur port Serial si = True

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

const byte charDegre = 3;
const byte charDegreDefinition[8] = {
0b00100,
0b01010,
0b00100,
0b00000,
0b00000,
0b00000,
0b00000,
0b00000};

byte dat[5];

// C++ code
//
void setup()
{
  //pinMode(LED_BUILTIN, OUTPUT);
  
  // initialisation de l'afficheur 
  LCD.init(); 
  LCD.backlight();
  LCD.clear();

  // Initialisation des caractères spéciaux
  LCD.createChar(charStop, charStopDefinition); 
  LCD.createChar(charPause, charPauseDefinition); 
  LCD.createChar(charPlay, charPlayDefinition); 
  LCD.createChar(charDegre, charDegreDefinition);
  
  // Initialisation Serial port pour le debug
  Serial.begin(9600);
  
  // Initialisation des entrées numériques
  pinMode(BtnStartPin, INPUT); 
  pinMode(BtnPlusPin, INPUT);
  pinMode(BtnMoinsPin, INPUT);
  pinMode(BtnResetPin, INPUT);
  pinMode(ShuntTemperature, INPUT);

  // Initialisation des sorties numériques
  pinMode(CmdRelayPin, OUTPUT);  
  pinMode(CmdFanPin, OUTPUT);  

  pinMode(capteurTempPin, OUTPUT);

  digitalWrite(CmdRelayPin, HIGH);
  digitalWrite(CmdFanPin, HIGH);
     
  // Initialisation des interruptions
  attachInterrupt(digitalPinToInterrupt(BtnStartPin), onEventStart, RISING); 
  attachInterrupt(digitalPinToInterrupt(BtnResetPin), onEventReset, RISING);   

  MesureTemperatureActive = digitalRead(ShuntTemperature); 
}

void loop()
{
  GestionDuMinuteur();
  
  // Mesure de la temperature toutes les secondes
  if(MesureTemperatureActive == 1 && cptCycles++ >=5)
  {
    GestionDeLaTemperature();
	  cptCycles = 0;
  }

   GestionAffichage();

  
  delay(200);
}

// Fonction appelée dans la boucle principale pour gérer les évènements et contrôles liés au fonctionnement du minuteur
void GestionDuMinuteur()
{
  // Pas assez d'interruptions pour gérer les 4 boutons, on se contente de regarder si les boutons "Plus" et "Moins" sont appuyés
  // Prise en compte de l'appui uniquement si l'état du compteur est sur "Arrêt"
  if(etatMinuteur == listeEtatsMinuteur_Arret )
  {
    if(digitalRead(BtnPlusPin) == HIGH && dureeSaisieEnMinutes < 99)
    {
      // Ajout d'une minute à la consigne
      dureeSaisieEnMinutes+=1;      
    }
    
	  if(digitalRead(BtnMoinsPin) == HIGH && dureeSaisieEnMinutes > 0)
    {
      // Retrait d'une minute à la consigne
      dureeSaisieEnMinutes-=1;
    }
  }

  // Si le minuteur est en marche on calcule et contrôle la durée écoulé
  if(etatMinuteur == listeEtatsMinuteur_Marche)
  {
    dureeEcouleeEnMillisecondes = millis() - valeurMillisDemarrageMinuteur;
	
	// Si on a atteint ou dépassé la consigne, on arrête le minuteyr
	if(dureeSaisieEnMilliSecondes <= dureeEcouleeEnMillisecondes)
	{
	  TraitementFinMinuteur();
	}
  }  
}

// Fonction appelée dans la boucle principale pour gérer les évènements et contrôles liés au contrôle de la température
void GestionDeLaTemperature()
{
  temperatureMesuree = LectureTemperature();
  
  if(temperatureMesuree > 25)
  {
    AllumageVentilation();
  }
  else
  {
    ArretVentilation();
  }
}

// Fonction appelée dans la boucle principale pour gérer l'affichage des infos sur l'écran LCD
void GestionAffichage()
{
  long tempsRestantSecondes = (dureeSaisieEnMilliSecondes - dureeEcouleeEnMillisecondes) / 1000;
  int minutesRestantes = tempsRestantSecondes / 60;
  int secondesRestantes = tempsRestantSecondes - minutesRestantes * 60;
  
  LCD.setCursor(0, 0);
  LCD.print("Delay: "); 
  LCD.print(dureeSaisieEnMinutes); 
  LCD.print(" min. "); 
      
  switch(etatMinuteur){
    case listeEtatsMinuteur_Arret :
      LCD.setCursor(0, 1);
      LCD.write((byte)charStop);
      LCD.setCursor(3, 1);
      LCD.print("     ");
      break;
    
    case listeEtatsMinuteur_Marche :
      LCD.setCursor(0, 1);
      LCD.write((byte)charPlay);
      
      LCD.setCursor(3, 1);
      LCD.print(minutesRestantes);
      LCD.print("\""); 
      LCD.print(secondesRestantes);
      //LCD.print("s  "); 
      break;
    
    case listeEtatsMinuteur_Pause :
      LCD.setCursor(0, 1);
      LCD.write((byte)charPause);
      break;    
  }

  if(MesureTemperatureActive == 1)
  {
    LCD.setCursor(11, 1);
        
    LCD.print(round(temperatureMesuree));
    LCD.write((byte)charDegre);
    LCD.print("C");
  }
}

void AllumageLampe(){
	// Pilotage  relais de puissance
	digitalWrite(CmdRelayPin, LOW);
}

void ArretLampe(){
	// Pilotage relais de puissance
	digitalWrite(CmdRelayPin, HIGH);
}

void AllumageVentilation()
{
	// Pilotage relais de puissance
	digitalWrite(CmdFanPin, LOW);
}

void ArretVentilation()
{
	// Pilotage relais de puissance
	digitalWrite(CmdFanPin, HIGH);
}

void TraitementFinMinuteur()
{
  etatMinuteur = listeEtatsMinuteur_Arret;
  dureeEcouleeEnMillisecondes = 0;
  ArretLampe();
}

// Appel lors de l'appui sur le bouton "Démarrage"
void onEventStart() 
{
  if(dureeSaisieEnMinutes == 0)
  {
    return;
  }

  if(etatMinuteur != listeEtatsMinuteur_Marche)
  {
    if(etatMinuteur == listeEtatsMinuteur_Arret)
	  {
      dureeSaisieEnMilliSecondes = dureeSaisieEnMinutes * 60 * 1000;
      valeurMillisDemarrageMinuteur = millis();
    }
    else
    {
    valeurMillisDemarrageMinuteur = millis() - dureeEcouleeEnMillisecondes;
    }
    etatMinuteur = listeEtatsMinuteur_Marche;
    AllumageLampe();    
  }
  else
  {
    etatMinuteur = listeEtatsMinuteur_Pause;
    ArretLampe();  
  }
}

// Appel lors de l'appui sur le bouton "Reset" (arret lampe + RAZ minuteur)
void onEventReset() 
{
  TraitementFinMinuteur();
}

float LectureTemperature()
{
  start_test();
  Serial.print("Humdity = ");
  Serial.print(dat[0], DEC); //Displays the integer bits of humidity;
  Serial.print('.');
  Serial.print(dat[1], DEC); //Displays the decimal places of the humidity;
  Serial.print("% - ");
  Serial.print("Temperature = ");
  Serial.print(dat[2], DEC); //Displays the integer bits of temperature;
  Serial.print('.');
  Serial.print(dat[3], DEC); //Displays the decimal places of the temperature;
  Serial.print("C ");
  byte checksum = dat[0] + dat[1] + dat[2] + dat[3];
  if (dat[4] != checksum) 
    Serial.println("-- Checksum Error!");
  else
    Serial.println("-- Checksum OK");

  float temperatureMesuree = dat[2];
  float decimalPart = dat[3];
  temperatureMesuree += decimalPart / 10;

  return temperatureMesuree;
}

void start_test()
{
  digitalWrite(capteurTempPin, LOW); //Pull down the bus to send the start signal
  delay(30); //The delay is greater than 18 ms so that DHT 11 can detect the start signal
  digitalWrite(capteurTempPin, HIGH);
  delayMicroseconds(40); //Wait for DHT11 to respond
  pinMode(capteurTempPin, INPUT);
  while(digitalRead(capteurTempPin) == HIGH);
  delayMicroseconds(80); //The DHT11 responds by pulling the bus low for 80us;
  
  if(digitalRead(capteurTempPin) == LOW)
    delayMicroseconds(80); //DHT11 pulled up after the bus 80us to start sending data;

  for(int i = 0; i < 5; i++) //Receiving temperature and humidity data, check bits are not considered;
    dat[i] = read_data();
	
  pinMode(capteurTempPin, OUTPUT);
  digitalWrite(capteurTempPin, HIGH); //After the completion of a release of data bus, waiting for the host to start the next signal
}

byte read_data()
{
  byte i = 0;
  byte result = 0;
  for (i = 0; i < 8; i++) 
  {
    while (digitalRead(capteurTempPin) == LOW); // wait 50us
    delayMicroseconds(30); //The duration of the high level is judged to determine whether the data is '0' or '1'
    if (digitalRead(capteurTempPin) == HIGH)
      result |= (1 << (8 - i)); //High in the former, low in the post
  
    while (digitalRead(capteurTempPin) == HIGH); //Data '1', waiting for the next bit of reception
  }
  return result;
}
