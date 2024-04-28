/* Z-Gesellschaft
   forteZZa3000
   common.h

100 Watts Output Thermoelectric Power Generator
Part Number TEG-HL100-12V-2
Heat source/Water temperature  270 / 30
Heat source/cooling Water Flow / cm3 / min 800
Matched output power / W 100
Open circuit voltage / VDC  24
Matched load output Voltage / VDC 12
Matched load output current / A 8.4
Dimension of the unit / mm 300× 155× 76
Specifications the inlet/ outlet pipe Φ10mm Two-Touch Fitting Φ10mm Two-Touch Fitting
Weight / Kg 3.0 3.0

Bismut-Tellurid
HOT SIDE MAX: 330°C
HOT SIDE KURZFRISTIG: 400°C
COLD SIDE MAX: 200°C -> dann kaputt.


*/
#include "Arduino.h"
#include "stdint.h"

#ifndef ZCOMMON
   #define ZCOMMON

//Dinge für den Motör
const int MOTOROUT_PIN = 7;
const int MOTORIN_PIN = 8;
const int MOTOR_ENABLE_PIN = 9;
const uint32_t MOTOR_LAUFZEIT = 25000;
const int TASTER_NAUF_LED_PIN = 6;
const int TASTER_NAB_LED_PIN = 5;

//Dinge für alle
const int LED = 13;
const int BUZZER_PIN = 10;
const int TASTER_NAUF_PIN = 2;
const int TASTER_NAB_PIN = 3;
const int16_t MAX_UNTEN_TEMP = 340;
const int16_t MAX_OBEN_TEMP = 80;
const int16_t WARN_UNTEN_TEMP = 330;
const int16_t WARN_OBEN_TEMP = 60;
const uint16_t BACKLIGHT_MILLIS = 30000;
const int TEG_VOLTAGE_PIN = A1;
const int TEG_AMPERE_PIN = A0;

#endif