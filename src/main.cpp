#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "HX711.h"
// Définitions

// pins encodeur
#define PIN_ENC1 2 // PD2 INT0
#define PIN_ENC2 3 // PD3 INT1
// pin bouton poussoir
#define PIN_BP1 0x10 // PD4
#define PIN_BP2 0x01 // PD0
// pin strip led
#define PIN_LEDS 5 // PD5
#define NUMPIXELS 10
// pins pompes
#define PIN_POMPE1 7 // PD6
#define PIN_POMPE2 6 // PB0 pin 8
// pins ecran
#define PIN_DC 9   // PB1
#define PIN_RST 10 // PB2
#define PIN_SDA 11 // PB3
#define PIN_SCK 13 // PB5
// pins capteur de poids
#define LOADCELL_SCK_PIN A5  // PD2
#define LOADCELL_DOUT_PIN A4 // PD3

// Objets
HX711 scale;
TFT_eSPI tft = TFT_eSPI();                                          // instanciation pour pouvoir utiliser les méthodes de TFT_eSPI
Adafruit_NeoPixel strip(NUMPIXELS, PIN_LEDS, NEO_GRB + NEO_KHZ800); // ligne de configuration des leds strips

// Variables Globales
float calibration_factor = -1094000; // facteur de calibration du capteur de poids
float g_float_poids;                 // Poids en g
int g_int_dose_liq1 = 50;           // Dose de liquide 1 en mL
int g_int_dose_liq2 = 50;           // Dose de liquide 2 en mL
int g_int_tare = 5;                  // Poids de la bouteille vide
bool g_bool_BP;                      // Etat du bouton poussoir
bool g_bool_etatPompe1;              // Etat de la pompe 1
bool g_bool_etatPompe2;              // Etat de la pompe 2
char g_char_etatcourant;
uint16_t g_uint16_encValue = 0;
int g_uint16_encValue_temp = 0;

typedef enum
{
  READY,  // etat initial de démarrage
  ATT_BP, // Attente de départ

  CLCLASSIQUE, // mode 1
  EDITMODE,    // mode 2
  DEUXPOMPES,  // mode 3

  TRANSCLASSEDIT,       // etat pour passer de classique à edit avec le bouton technicien
  TRANSEDITDEUXPOMPES,  // etat pour passer de edit à deuxpompes avec le bouton technicien
  TRANSDEUXPOMPESCLASS, // etat pour passer de deuxpompes à classique avec le bouton technicien

  ATTEDIT,          // att avant de rentrer dans le mode edition
  ATTVALIDQUANTITY, // att avant de sortir du mode edition

  EDIT1, // les 2 etats alternant entre eux pour que la valeur soit rafraîchie après chaque modification

  ATTDEUXPOMPES,

  ATTCLASSIQUE, // att avant de passer au service de la boisson pour le mode classique

  SERVICE1, // service de la première pompe
  SERVICE2, // service de la deuxième pompe

  FIN // fin avec possibilité de revenir au début

} TEtat;
// Variables : état courant et état précédent
TEtat g_etat_courant, g_etat_old;

bool BPSTATE(void)
{
  bool g_bool_BP = !((PIND & PIN_BP1) == PIN_BP1);
  return g_bool_BP;
}
bool BPSTATE2(void)
{
  bool g_bool_BP = !((PIND & PIN_BP2) == PIN_BP2);
  return g_bool_BP;
}
void allumeLedsVert()
{
  for (int iBcl = 0; iBcl < NUMPIXELS; iBcl++)
  {
    strip.setPixelColor(iBcl, strip.Color(0, 150, 0));
    strip.show();
  }
}
void allumeLedsBleu()
{
  for (int iBcl = 0; iBcl < NUMPIXELS; iBcl++)
  {
    strip.setPixelColor(iBcl, strip.Color(0, 20, 150));
    strip.show();
  }
}
void allumeLedsRouge()
{
  for (int iBcl = 0; iBcl < NUMPIXELS; iBcl++)
  {
    strip.setPixelColor(iBcl, strip.Color(150, 0, 0));
    strip.show();
  }
}
void allumeLedsLiq(int x_int_numLeds)
{
  for (int iBcl = 0; iBcl < x_int_numLeds; iBcl++)
  {
    strip.setPixelColor(iBcl, strip.Color(150, 0, 150));
    strip.show();
  }
}
void eteintLeds()
{
  for (int iBcl = 0; iBcl < NUMPIXELS; iBcl++)
  {
    strip.setPixelColor(iBcl, strip.Color(0, 0, 0));
    strip.show();
  }
}
void DisablePompe1()
{
  PORTD |= (1 << PIN_POMPE1);
}
void DisablePompe2()
{
  PORTD |= (1 << PIN_POMPE2);
}
void EnablePompe1()
{
  PORTD &= ~(1 << PIN_POMPE1);
}
void EnablePompe2()
{
  PORTD &= ~(1 << PIN_POMPE2);
}
void IOConfig()
{
  DDRB = 0x01;  // premiere sortie pompe 2 pb0
  DDRC = 0x00;
  DDRD = 0b11100001; // bit 0 bouton du technicien, et les dernier bits pour les pompes de les leds
}
void ScreenConfig()
{

  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_DARKGREEN);
  tft.setTextColor(TFT_WHITE);
  tft.drawRightString("Initializing...", 128, 64, 2);
}
void EncodeurConfig()
{
  EICRA = B00000000;
  EIMSK = B00000001;
}
void WeightSensorConfig()
{
  // Initialisation du capteur de poids

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale();
  scale.tare(); // Reset the scale to 0
}

void setup()
{
  g_etat_courant = READY;
  g_etat_old = g_etat_courant;
  // Initialisation des ports en entrée ou sortie
  Serial.begin(9600); // pour débogage et capteur de poids
  IOConfig();
  ScreenConfig();
  WeightSensorConfig();
  EncodeurConfig();
  strip.begin();

  // long zero_factor = scale.read_average(); // Get a baseline reading
  //  Serial.print("Zero factor: ");           // This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  //  Serial.println(zero_factor);
}

void loop()
{
  // VARIABLES LOCALES
  int l_int_numLeds;

  bool g_bool_BP1 = BPSTATE();

  switch (g_etat_courant)
  {
  case READY:
    if (g_bool_BP1 == true)
    {
      g_etat_courant = ATT_BP;
    }
    break;
  case ATT_BP:
    if (g_bool_BP1 == false)
    {
      g_etat_courant = CLCLASSIQUE;
    }
    break;
  case CLCLASSIQUE:
    if (g_bool_BP1 == true)
    {
      g_etat_courant = ATTCLASSIQUE;
    }
    else if (BPSTATE2() == true)
    {
      g_etat_courant = TRANSCLASSEDIT;
    }
    else if ((20 <= g_uint16_encValue) & (g_uint16_encValue < 120))
    {

      g_etat_courant = EDITMODE;
    }
    break;
  case TRANSCLASSEDIT:
    if (BPSTATE2() == false)
    {
      g_etat_courant = EDITMODE;
    }
    break;

  case EDITMODE:
    if (g_bool_BP1 == true)
    {
      g_etat_courant = ATTEDIT;
    }
    else if ((BPSTATE2() == true))
    {
      g_etat_courant = TRANSEDITDEUXPOMPES;
    }
    else if (((120 <= g_uint16_encValue) & (g_uint16_encValue < 240)))
    {

      g_etat_courant = DEUXPOMPES;
    }
    break;
  case TRANSEDITDEUXPOMPES:
    if (BPSTATE2() == false)
    {
      g_etat_courant = DEUXPOMPES;
    }
    break;
  case TRANSDEUXPOMPESCLASS:
    if (BPSTATE2() == false)
    {
      g_etat_courant = CLCLASSIQUE;
    }
    break;
  case DEUXPOMPES:
    if ((BPSTATE2() == true))
    {
      g_etat_courant = TRANSDEUXPOMPESCLASS;
    }
    else if (((240 <= g_uint16_encValue) & (g_uint16_encValue < 360)))
    {
      g_etat_courant = CLCLASSIQUE;
    }
    else if (g_bool_BP1 == true)
    {
      g_etat_courant = ATTDEUXPOMPES;
    }
    break;
  case ATTDEUXPOMPES:
    if (g_bool_BP1 == false)
    {
      g_etat_courant = SERVICE1;
    }
    break;
  case ATTEDIT:
    if (g_bool_BP1 == false)
    {
      g_etat_courant = EDIT1;
    }
    break;
  case EDIT1:
    if (g_bool_BP1 == true)
    {
      g_etat_courant = ATTVALIDQUANTITY;
    }
    break;

  case ATTVALIDQUANTITY:
    if (g_bool_BP1 == false)
    {
      g_etat_courant = SERVICE1;
    }
  case ATTCLASSIQUE:
    if (g_bool_BP1 == false)
    {
      g_etat_courant = SERVICE1;
    }
    break;
  case SERVICE1:
    if (g_float_poids >= g_int_tare + g_int_dose_liq1)
    {
      g_etat_courant = SERVICE2;
    }
    break;
  case SERVICE2:
    if (g_float_poids >= g_int_tare + g_int_dose_liq1 + g_int_dose_liq2 + 1)
    {
      g_etat_courant = FIN;
    }
    break;
  case FIN:
    if (g_float_poids < g_int_tare)
    {
      g_etat_courant = READY;
    }
  }
  // MAJ SORTIES
  switch (g_etat_courant)
  {
  case READY:

    scale.tare(); // remets la balance à 0 pour éviter d'avoir le poids négatif
    g_bool_etatPompe1 = false;
    g_bool_etatPompe2 = false;
    allumeLedsBleu();
    tft.fillScreen(TFT_BLUE);
    tft.setTextColor(TFT_WHITE);
    tft.drawRightString("red button to start :)", 140, 64, 2);
    tft.drawRightString("ETAT : READY", 128, 0, 2);
    break;
  case ATT_BP:
    g_bool_etatPompe1 = false;
    g_bool_etatPompe2 = false;
    eteintLeds();
    g_uint16_encValue = 0;
    g_uint16_encValue_temp = g_uint16_encValue;
    tft.fillScreen(TFT_BLACK);
    tft.drawRightString("ETAT : ATT_BP", 128, 0, 2);
    break;
  case CLCLASSIQUE:
    g_bool_etatPompe1 = false;
    g_bool_etatPompe2 = false;
    tft.fillScreen(TFT_DARKCYAN);
    tft.setTextColor(TFT_WHITE);
    tft.drawRightString("CLASSIC MODE", 128, 64, 2);
    allumeLedsRouge();
    break;
  case EDITMODE:
    g_bool_etatPompe1 = false;
    g_bool_etatPompe2 = false;
    allumeLedsVert();
    tft.fillScreen(TFT_DARKGREEN);
    tft.setTextColor(TFT_WHITE);
    tft.drawRightString("EDIT MODE", 128, 64, 2);
    break;
  case ATTEDIT:
    g_uint16_encValue = 0;
    tft.fillScreen(TFT_BLACK);
    tft.drawRightString("ETAT : ATTEDIT", 128, 0, 2);
    break;
  case EDIT1:
    g_bool_etatPompe1 = false;
    g_bool_etatPompe2 = false;
    tft.drawRightString("ETAT : EDIT 1", 128, 0, 2);
    tft.fillScreen(TFT_BLACK);
    tft.drawFloat(g_uint16_encValue, g_uint16_encValue, 64, 20);
    break;
  case ATTVALIDQUANTITY:
    g_int_dose_liq2 = 0; // pas de deuxième pompe dans ce mode
    g_int_dose_liq1 = g_uint16_encValue;
    tft.fillScreen(TFT_BLACK);
    tft.drawRightString("ETAT : ATTVALIDQUANTITY", 128, 0, 2);
    break;
  case DEUXPOMPES:
    g_bool_etatPompe1 = false;
    g_bool_etatPompe2 = false;
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_WHITE);
    tft.drawRightString("2 POMPES MODE", 128, 64, 2);
    tft.drawRightString("ETAT : DEUXPOMPES", 128, 0, 2);

    break;
  case ATTDEUXPOMPES:
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.drawRightString("ETAT : ATTDEUXPOMPES", 128, 0, 2);
    break;

  case ATTCLASSIQUE:
    g_bool_etatPompe1 = false;
    g_bool_etatPompe2 = false;
    g_int_dose_liq2 = 0;
    allumeLedsRouge();
    tft.fillScreen(TFT_BLACK);
    tft.drawRightString("ETAT : ATTCLASSIQUE", 128, 0, 2);
    break;
  case SERVICE1:
    //tft.fillScreen(TFT_RED);
    //tft.setTextColor(TFT_WHITE);
    //tft.drawRightString("Service de la pompe 1 en cours", 64, 64, 2);
    //tft.drawRightString("ETAT : SERVICE1", 128, 0, 2);
    scale.set_scale(calibration_factor); // Adjust to this calibration factor
    // Serial.print("Reading: ");
    //  Lecture du poids
    g_float_poids = scale.get_units() * 1000;
    // Affichage du poids
    // tft.drawRightString("le poids est :", 64, 20, 2);
    // tft.drawFloat(g_float_poids, 0, 64, 20);
    // tft.drawRightString("grammes", 64, 20, 2);
    l_int_numLeds = map(g_float_poids, g_int_tare, g_int_tare + g_int_dose_liq1 + g_int_dose_liq2, 0, NUMPIXELS);

    allumeLedsLiq(l_int_numLeds);
    g_bool_etatPompe1 = false;
    g_bool_etatPompe2 = true;

    break;

  case SERVICE2:
    // LED MAPS
    // tft.drawRightString("le poids est :", 64, 20, 2);
    //tft.fillScreen(TFT_BLACK);
    //tft.setTextColor(TFT_WHITE);
    //tft.drawRightString("ETAT : SERVICE2", 128, 0, 2);
    //tft.drawRightString("Service de la pompe 2 en cours", 64, 64, 2);
    g_float_poids = scale.get_units() * 1000;
    //tft.drawFloat(g_float_poids, 0, 64, 20);
    // tft.drawRightString("grammes", 64, 20, 2);

    l_int_numLeds = map(g_float_poids, g_int_tare, g_int_tare + g_int_dose_liq1 + g_int_dose_liq2, 0, NUMPIXELS);
    allumeLedsLiq(l_int_numLeds);

    g_bool_etatPompe1 = true;
    g_bool_etatPompe2 = false;

    break;

  case FIN:
    g_float_poids = scale.get_units() * 1000;
    g_bool_etatPompe1 = false;
    g_bool_etatPompe2 = false;
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_WHITE);
    tft.drawRightString("ETAT : FIN", 128, 0, 2);
    tft.drawRightString("Retirez votre recipient ", 128, 64, 2);
    tft.drawRightString("et à bientot", 64, 50, 2);
    allumeLedsVert();
    break;

  default:
    break;
  }

  // MAJ POMPES
  if (g_bool_etatPompe1 == false)
  {
    DisablePompe1();
  }
  if (g_bool_etatPompe2 == false)
  {
    DisablePompe2();
  }
  if (g_bool_etatPompe1 == true)
  {
    EnablePompe1();
  }
  if (g_bool_etatPompe2 == true)
  {
    EnablePompe2();
  }

  // DEBUG
  /*
  Serial.print("Etat : ");
  Serial.println(g_etat_courant);
  Serial.print("Poids : ");
  Serial.println(g_float_poids);
  Serial.print("Valeur encodeur : ");
  Serial.println(g_uint16_encValue);
  Serial.print("booleen temps 1 sec : ");
  Serial.println(g_bool_temps);
  Serial.print("etat bouton 1 :");
  Serial.println(BPSTATE());
  Serial.print("etat bouton 2 :");
  Serial.println(BPSTATE2());
  delay(1000);
  */
}

ISR(INT0_vect)
{
  static int8_t lookup_table[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
  static uint8_t enc_val = 0;

  enc_val = enc_val << 2;
  enc_val = enc_val | ((PIND & 0b1100) >> 2);

  g_uint16_encValue = g_uint16_encValue + (lookup_table[enc_val & 0b1111] * 10);

  // limites à ne pas dépasser
  if (g_uint16_encValue < 10)
  {
    g_uint16_encValue = 10;
  }
  else if (g_uint16_encValue > 370)
  {
    g_uint16_encValue = 370;
  }
}

