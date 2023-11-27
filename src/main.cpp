#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#define PIN_POMPE1 6
#define PIN_POMPE2 7
#define PIN_POTENTIOMETRE 1//A1
#define PIN_BP1 0x10
#define PIN_POIDS 2
#define PIN_LEDS 6
#define NUMPIXELS 10

int g_int_poids;
int g_int_dose_liq1 = 400;
int g_int_dose_liq2 = 200;
int g_int_tare = 50;
bool g_bool_BP;
bool g_bool_etatPompe1;
bool g_bool_etatPompe2;

typedef enum {READY,ATT_BP,SERVICE1,SERVICE2,FIN} TEtat;

Adafruit_NeoPixel strip(NUMPIXELS, PIN_LEDS, NEO_GRB + NEO_KHZ800);
TEtat g_etat = READY;

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

void setup() {
  Serial.begin(9600);
  DDRB = 0x00;
  DDRC = 0x00;
  DDRD = 0x00;
  strip.begin();
  
}

void loop() {
//VARIABLES LOCALES
  int l_int_numLeds;


  //LECTURE ENTREES

  //Etat du bouton cablé sur PORT Dx = PDx
  g_bool_BP = ((PIND & PIN_BP1) == PIN_BP1);

  //debug poids
  g_int_poids = analogRead(PIN_POTENTIOMETRE);



  //MAJ ETAT
  switch(g_etat){
    case READY :
      if(g_bool_BP==true){
        g_etat = ATT_BP; 
      }
      break; 
    case ATT_BP :
      if(g_bool_BP==false){
        g_etat = SERVICE1; 
      }
      break;
    case SERVICE1 :
      if(g_int_poids >= g_int_tare + g_int_dose_liq1){
        g_etat = SERVICE2;
      }
      break;
    case SERVICE2 : 
      if(g_int_poids >= g_int_tare + g_int_dose_liq1 + g_int_dose_liq2){
        g_etat = FIN;
      }
      break;
    case FIN :
      if(g_int_poids<g_int_tare){
        g_etat = READY;
      }
  }


  //MAJ SORTIES
  switch(g_etat){
    case READY :
      g_bool_etatPompe1 = false;
      g_bool_etatPompe2 = false;
      allumeLedsBleu();
      break;
    
    case ATT_BP :
      eteintLeds();
      break;
    
    case SERVICE1 :
      //LED MAPS
      l_int_numLeds = map(g_int_poids, g_int_tare, g_int_tare + g_int_dose_liq1 + g_int_dose_liq2, 0, NUMPIXELS);
      //Serial.print("Num leds : ");
      //Serial.println(l_int_numLeds);
      allumeLedsLiq(l_int_numLeds);
      g_bool_etatPompe1 = true;
      g_bool_etatPompe2 = false;
      break;
    
    case SERVICE2 :
      //LED MAPS
      l_int_numLeds = map(g_int_poids, g_int_tare, g_int_tare + g_int_dose_liq1 + g_int_dose_liq2, 0, NUMPIXELS);
      allumeLedsLiq(l_int_numLeds);
      g_bool_etatPompe1 = false;
      g_bool_etatPompe2 = true;
      break;
    
    case FIN :
      g_bool_etatPompe1 = false;
      g_bool_etatPompe2 = false;
      allumeLedsVert();
      break;

    default :
      break;
  }

  Serial.print("Etat : ");
  Serial.println(g_etat);
  Serial.print("Poids : ");
  Serial.println(g_int_poids);
  delay(1);
}
