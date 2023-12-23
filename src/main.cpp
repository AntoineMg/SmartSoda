#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include "HX711.h"
#include <TFT.h>  // Arduino LCD library
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> // Arduino SPI library

//Définitions

//pins capteur de poids
#define LOADCELL_SCK_PIN  0 //PD2
#define LOADCELL_DOUT_PIN  1 //PD3

//pins encodeur
#define PIN_ENCA 2 //PD0
#define PIN_ENCB 3 //PD1

//pin bouton poussoir
#define PIN_BP1 0x10 //PD4

//pin strip led
#define PIN_LEDS 5 //PD5

//pins pompes
#define PIN_POMPE1 6 //PD6
#define PIN_POMPE2 7 //PD7

//pins ecran
#define PIN_CS 8 //PB0
#define PIN_DC 9 //PB1
#define PIN_RST 10 //PB2
#define PIN_SDA 11 //PB3
#define PIN_LIGHT 12 //PB4
#define PIN_SCK 13 //PB5

//Definitions autres
#define NUMPIXELS 10 //nombre de leds
#define ANTI_BOUNCE 10 //anti rebond en ms

//Variables Globales
float calibration_factor = -1094000; //facteur de calibration du capteur de poids
float g_float_poids; //Poids en g
int g_int_dose_liq1; //Dose de liquide 1 en mL
int g_int_dose_liq2; //Dose de liquide 2 en mL
int g_int_tare = 10; //Poids à vide
bool g_bool_BP; //Etat du bouton poussoir
bool g_bool_etatPompe1;  //Etat de la pompe 1
bool g_bool_etatPompe2; //Etat de la pompe 2
bool sens; //Sens de rotation de l'encodeur

int g_int_dosage1[2] = {40,200}; //Classique
int g_int_dosage2[2] = {30,200}; //Léger
int g_int_dosage3[2] = {50,150}; //Fort

//Etats
typedef enum
{
  INIT,
  DOSAGE1,
  DOSAGE2,
  DOSAGE3,
  ATT_BP,
  SERVICE1,
  SERVICE2,
  FIN
} TEtat;

//Objets et instances
HX711 scale;
Adafruit_NeoPixel strip(NUMPIXELS, PIN_LEDS, NEO_GRB + NEO_KHZ800); //Objet strip led
TFT_eSPI tft = TFT_eSPI(); // instanciation pour pouvoir utiliser les méthodes de TFT_eSPI
TEtat g_etat_courant, g_etat_old; 

void allumeLedsVert(){
  for (int iBcl=0;iBcl<NUMPIXELS;iBcl++){
      strip.setPixelColor(iBcl, strip.Color(0, 150, 0));
      strip.show();
    }
}

void allumeLedsBleu(){
  for (int iBcl=0;iBcl<NUMPIXELS;iBcl++){
      strip.setPixelColor(iBcl, strip.Color(0, 20, 150));
      strip.show();
    }
}

void allumeLedsLiq(int x_int_numLeds){
  for (int iBcl=0;iBcl<x_int_numLeds;iBcl++){
      strip.setPixelColor(iBcl, strip.Color(150, 0, 150));
      strip.show();
    }
}

void eteintLeds(){
  for (int iBcl=0;iBcl<NUMPIXELS;iBcl++){
      strip.setPixelColor(iBcl, strip.Color(0, 0, 0));
      strip.show();
    }
}

void allumePompe1(){
  digitalWrite(PIN_POMPE1, LOW);
  //Serial.println("Pompe 1 allumée");
}

void allumePompe2(){
  digitalWrite(PIN_POMPE2, LOW);
  //Serial.println("Pompe 2 allumée");
}

void eteintPompe1(){
  digitalWrite(PIN_POMPE1, HIGH);
  //Serial.println("Pompe 1 éteinte");
}

void eteintPompe2(){
  digitalWrite(PIN_POMPE2, HIGH);
  //Serial.println("Pompe 2 éteinte");
}

void encodeurTickA()
{
  static unsigned long dateDernierChangement = 0;

  unsigned long date = millis();
  if ((date - dateDernierChangement) > ANTI_BOUNCE)
  {
    if (digitalRead(PIN_ENCB) == LOW)
    {
      sens = true;
    }
    else
    {
      sens = false;
    }
    dateDernierChangement = date;
  }
}

void setup() {
  //Initialisation des ports en entrée ou sortie
  Serial.begin(9600);
  DDRB = 0x00;
  DDRC = 0x00;
  DDRD = 0x00;
  pinMode(PIN_POMPE1, OUTPUT);
  pinMode(PIN_POMPE2, OUTPUT);
  strip.begin();

  g_etat_courant = INIT; //Etat initial
  g_etat_old = g_etat_courant; 

  //Initialisation de l'écran
  tft.init();   
  tft.setRotation(0);   

  //Initialisation du capteur de poids
  Serial.println("HX711 calibration sketch");
  Serial.println("Remove all weight from scale");
  Serial.println("After readings begin, place known weight on scale");
  Serial.println("Press + or a to increase calibration factor");
  Serial.println("Press - or z to decrease calibration factor");

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale();
  scale.tare();	//Reset the scale to 0

  long zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);

  //Encodeur rotatif
  pinMode(PIN_ENCA, INPUT);
  pinMode(PIN_ENCB, INPUT);

  attachInterrupt(0, encodeurTickA, FALLING);

  
}

void loop() {
  //VARIABLES LOCALES
  int l_int_numLeds;

  //LECTURE ENTREES
  scale.set_scale(calibration_factor); //Adjust to this calibration factor
  Serial.print("Reading: ");

  //Lecture du poids
  g_float_poids = scale.get_units()*1000;

  //Affichage du poids
  Serial.print(g_float_poids);
  Serial.print(" g"); //Change this to kg and re-adjust the calibration factor if you follow SI units like a sane person
  Serial.print(" calibration_factor: ");
  Serial.print(calibration_factor);
  Serial.println();

  //Etat du bouton cablé sur PORT Dx = PDx
  g_bool_BP = !((PIND & PIN_BP1) == PIN_BP1);

  //debug poids via potentiomètre
  //g_float_poids = analogRead(PIN_POTENTIOMETRE);



  //MAJ ETAT
  switch(g_etat_courant){
    //INIT
    case INIT:
    if (g_bool_BP){
      g_etat_courant = DOSAGE1; // si on pose un verre (donc verre détecté) et qu'on appuie sur le bouton valider alors on passe au mode dosage 1
    }
    break;

    //DOSAGE1
    case DOSAGE1:
      if (sens==true){
        g_etat_courant = DOSAGE2; 
      }
      else if (g_bool_BP){
        g_etat_courant = ATT_BP; 
      }
      break;
    
    //DOSAGE2
    case DOSAGE2:
      if (sens==false){
        g_etat_courant = DOSAGE1; 
      }

      else if (sens==true){
        g_etat_courant = DOSAGE3; 
      }

      else if (g_bool_BP){
        g_etat_courant = ATT_BP; 
      }
      break;
    
    //DOSAGE3
    case DOSAGE3:
      if (sens==false){
        g_etat_courant = DOSAGE2; 
      }

      else if (g_bool_BP){
        g_etat_courant = ATT_BP; 
      }
      break;

    // ATT_BP
    case ATT_BP :
      if(g_bool_BP==false){
        g_etat_courant = SERVICE1; 
      }
      break;
    
    // SERVICE1
    case SERVICE1 :
      if(g_float_poids >= g_int_tare + g_int_dose_liq1){
        g_etat_courant = SERVICE2;
      }
      break;
    
    // SERVICE2
    case SERVICE2 : 
      if(g_float_poids >= g_int_tare + g_int_dose_liq1 + g_int_dose_liq2){
        g_etat_courant = FIN;
      }
      break;
    
    // FIN
    case FIN :
      if(g_float_poids<g_int_tare){
        g_etat_courant = DOSAGE1;
      }
  }

  if(g_etat_courant != g_etat_old){
    //MAJ SORTIES
    switch(g_etat_courant){

      //INIT
      case INIT:
        //Actions d'initialisation
        g_bool_etatPompe1 = false;
        g_bool_etatPompe2 = false;
        allumeLedsBleu();
        break;
      
      case DOSAGE1:
        //Actions de mode Dosage 1
        g_int_dose_liq1 = g_int_dosage1[0];
        g_int_dose_liq2 = g_int_dosage1[1];
        allumeLedsBleu();
        break;
      
      case DOSAGE2:
        //Actions de mode Dosage 2
        g_int_dose_liq1 = g_int_dosage2[0];
        g_int_dose_liq2 = g_int_dosage2[1];
        allumeLedsBleu();
        break;
      
      case DOSAGE3:
        //Actions de mode Dosage 3
        g_int_dose_liq1 = g_int_dosage3[0];
        g_int_dose_liq2 = g_int_dosage3[1];
        allumeLedsBleu();
        break;
      
      case ATT_BP :
        //Actions de mode Attente BP
        eteintLeds();
        break;
      
      case SERVICE1 :
        //Actions de mode Service 1
        //LED MAPS
        l_int_numLeds = map(g_float_poids, g_int_tare, g_int_tare + g_int_dose_liq1 + g_int_dose_liq2, 0, NUMPIXELS);
        allumeLedsLiq(l_int_numLeds);

        //Changement des états des pompes
        g_bool_etatPompe1 = true;
        g_bool_etatPompe2 = false;
        break;
      
      case SERVICE2 :
        //Actions de mode Service 2
        //LED MAPS
        l_int_numLeds = map(g_float_poids, g_int_tare, g_int_tare + g_int_dose_liq1 + g_int_dose_liq2, 0, NUMPIXELS);
        allumeLedsLiq(l_int_numLeds);

        //Changement des états des pompes
        g_bool_etatPompe1 = false;
        g_bool_etatPompe2 = true;
        break;
      
      case FIN :
        //Actions de mode Fin
        g_bool_etatPompe1 = false;
        g_bool_etatPompe2 = false;

        allumeLedsVert();
        break;

      default :
        break;
    }

    //MAJ POMPES
    if(g_bool_etatPompe1==false){
      eteintPompe1();
    }
    if(g_bool_etatPompe2==false){
      eteintPompe2();
    }
    if(g_bool_etatPompe1==true){
      allumePompe1();
    }
    if(g_bool_etatPompe2==true){
      allumePompe2();
    }
  }


  //DEBUG
  Serial.print("Etat : ");
  Serial.println(g_etat_courant);
  Serial.print("Poids : ");
  Serial.println(g_float_poids);
  delay(100);
}
