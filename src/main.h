/* Z-Gesellschaft forteZZa3000
*  DER Sicherheitsregulator für Thermoelektrische Generatoren
*  Erstellt 04/2024
*
*
*/
#include "Arduino.h"
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <string.h>
#include <stdint.h>
#include "forteZZaTemp.h"
#include "motor.h"
#include "common.h"

#define START_ADDR 0
#define OFFSET_ADDR 10
#define NABTIME_ADDR 20
#define NAUFTIME_ADDR 30
#define HUBMS_ADDR 40

const byte THERMOCOUPLE_PIN = 4;
const byte DS18B20_PIN = 16;

//0,1,2,\n!!!! ->depp
char zahl[3];
char lcdstring[20];
uint8_t naufGedrueckt = 0;
uint8_t nabGedrueckt = 0;
int16_t tegTemp=0;
int16_t kuehlTemp=0;
uint32_t oldMillis = millis();
uint32_t backLightMillis = millis();
uint16_t warnStatus = 0;
uint16_t oldWarnStatus = 0;
uint32_t warnMillis = 0;
volatile uint32_t nabPressMillis = 0;
volatile uint32_t naufPressMillis = 0;
uint32_t doppelKlickTime = millis();
enum WARNSTATE{
    NORMAL, WARNUNG, WAYTOHOT
};



//Custom Chars für das LCD
byte tempObenChar[] = {0x1F, 0x0E, 0x04, 0x00, 0x1F, 0x0E, 0x1F, 0x00};
byte gradCChar[] = {0x08, 0x14, 0x08, 0x06, 0x09, 0x08, 0x09, 0x06};  
byte deltaTChar[] = {0x04, 0x0A, 0x11, 0x1F, 0x00, 0x0E, 0x04, 0x04};
byte tempUntenChar[] = {0x00, 0x1F, 0x0E, 0x1F, 0x00, 0x04, 0x0E, 0x1F};

// put function declarations here:
void checkTemp(void);
void kalibrieren(void);
void zeigTemperatur(void);
void sirene(uint8_t howOften);
void sirene2(uint8_t howOften);
void colt(int buzzerPin);
void colt2(int buzzerPin);
void snuggles(int buzzerPin);
void nabINT();
void naufINT();
void powermessung();
void ausgabe(uint8_t zeile, const char* text);
void ausgabe(uint8_t zeile, const __FlashStringHelper * text);

