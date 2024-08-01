/* Z-Gesellschaft
 *  forteZZa3000 (R)
 *  Sicherheitsregulator für ofenbasierte Thermoelektrische Generatoren
 *
 */
#include "main.h"
#include "EEPROM.h"
#include <LiquidCrystal_I2C.h>

/*
 * TODO: bei >50W -> colt
 *       bei >5W -> snuggles+display an
 *       taste gedrückt: stop motor!!!!
 *       Thermocouple offset bei Start an DS18B20 ausrichten

 */

// LIBS
LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display
forteZZaTemp fT(DS18B20_PIN, THERMOCOUPLE_PIN, &SPI);
motor zMotor;

void setup() {
	// put your setup code here, to run once:
	// start serial port
	Serial.begin(9600);
	Serial.println(F("forteZZa3000"));

	// Temperatursensoren
	SPI.begin();
	fT.begin();
	fT.setSPIspeed(2000);
	fT.setOffset(0); // ThermoCouple Offset erstmal auf 0
	zMotor.begin(&Serial);

	// PINS herrichten
	pinMode(TASTER_NAB_PIN, INPUT_PULLUP);
	pinMode(TASTER_NAUF_PIN, INPUT_PULLUP);
	pinMode(BUZZER_PIN, OUTPUT);
	pinMode(TEG_AMPERE_PIN, INPUT);
	pinMode(TEG_VOLTAGE_PIN, INPUT);

	lcd.init(); // initialize the lcd
	// Print a message to the LCD.
	lcd.noBacklight();
	lcd.backlight();
	ausgabe(0, F("forteZZa3000"));
	ausgabe(1, F("Z-Gesellschaft"));

	//Read Data from EEPROM
	fT.setOffset(eeprom_read_word((uint16_t*) (START_ADDR + OFFSET_ADDR))); //Temp Offset
	zMotor.setLaufzeitNab(
			eeprom_read_dword((uint32_t*) (START_ADDR + NABTIME_ADDR)));
	zMotor.setLaufzeitNauf(
			eeprom_read_dword((uint32_t*) (START_ADDR + NAUFTIME_ADDR)));
	zMotor.setHubMs(eeprom_read_word((uint16_t*) (START_ADDR + HUBMS_ADDR)));

	// Sirene mal testen
	sirene(1);

	// Dr. Snuggles huldigen
	// snuggles(BUZZER_PIN);
	// Motor kalibrieren

	if (!digitalRead(TASTER_NAB_PIN) || !digitalRead(TASTER_NAUF_PIN)) {
		delay(1000);
		kalibrieren();
	}

	// Interrupt erst nach Kalibrierung!
	attachInterrupt(digitalPinToInterrupt(TASTER_NAB_PIN), nabINT, FALLING);
	attachInterrupt(digitalPinToInterrupt(TASTER_NAUF_PIN), naufINT, FALLING);

	lcd.clear();
	lcd.createChar(0, tempObenChar);
	lcd.setCursor(0, 0);
	lcd.write(0);
	lcd.createChar(1, gradCChar);
	lcd.setCursor(3, 0);
	lcd.write(1);
	lcd.createChar(2, deltaTChar);
	lcd.setCursor(5, 0);
	lcd.write(2);
	lcd.setCursor(9, 0);
	lcd.write(1);
	lcd.createChar(3, tempUntenChar);
	lcd.setCursor(11, 0);
	lcd.write(3);
	lcd.setCursor(15, 0);
	lcd.write(1);
	fT.request();
}

void loop() {

	Serial.print("MS:");
	Serial.print(zMotor.getMotorState());
	Serial.print(" MR:");
	Serial.print(zMotor.motorR);
	Serial.print(" %:");
	Serial.print(zMotor.getProzent());
	Serial.print(" hub:");
	Serial.print(zMotor.getHub());
	Serial.print(" WarnS:");
	Serial.print(warnStatus);
	Serial.print(" nabgedr:");
	Serial.print(nabGedrueckt);
	Serial.print(" naufgedr:");
	Serial.print(naufGedrueckt);
	Serial.print(" lzup:");
	Serial.print(zMotor._laufzeitNauf);
	Serial.print(" lzNab:");
	Serial.print(zMotor._laufzeitNab);
	Serial.print(" ms:");
	Serial.println(zMotor._msProMmHub);

	zMotor.check();

	if (zMotor.getMotorState() == zMotor.RUNNING) {
			sprintf_P(lcdstring, PSTR("%3d %% %3d mm    "), zMotor.getProzent(),
					zMotor.getHub());
			ausgabe(1, lcdstring);
		} else {
			if (millis() - oldMillis > 500) {
				oldMillis = millis();
				fT.request();
				delay(20);
				checkTemp();
				zeigTemperatur();
				powermessung();
			}

		}



	if (powerW > 5000) {
		lcd.backlight();
	} else if (powerW < 5000 && millis() - backLightMillis > BACKLIGHT_MILLIS) {
		lcd.noBacklight();
	}

	if (nabGedrueckt || naufGedrueckt) {
		lcd.backlight();
		backLightMillis = millis();
		// Das Ding soll jetzt Stoppen
		if (zMotor.motorS == zMotor.RUNNING) {
			if (readTasten(TASTER_NAB_PIN, TASTER_NAUF_PIN)) {
				zMotor.setMotorState(zMotor.STOP);
				Serial.println(F("SOLL STOPPEN"));
				nabGedrueckt = 0;
				naufGedrueckt = 0;
			}
		}

	}

	if (nabGedrueckt && warnStatus == NORMAL) {
		uint16_t tastenZeit = readTastenTime(TASTER_NAB_PIN);
		// Ding ist schon unten, aber nicht ganz
		if (tastenZeit > 1000 && zMotor.motorR == zMotor.IST_UNTEN) {
			ausgabe(1, F("EXTRA HINAB     "));
			zMotor.setMotorState(zMotor.RUNTEREXTRA);
		} else if (tastenZeit > 50
				&& (zMotor.motorR == zMotor.IST_OBEN
						|| zMotor.motorR == zMotor.STILL)) {
			// Ding ist Oben, soll nach unten
			zMotor.setSollHub(0);
			zMotor.setMotorState(zMotor.RUNTER);
		}

		nabGedrueckt = 0;
	}

	if (naufGedrueckt) {
		uint16_t tastenZeit = readTastenTime(TASTER_NAUF_PIN);
		if (tastenZeit > 1000 && zMotor.motorR == zMotor.IST_OBEN) {
			ausgabe(1, F("EXTRA HINAUF    "));
			zMotor.setMotorState(zMotor.RAUFEXTRA);
		} else if (tastenZeit > 50
				&& (zMotor.motorR == zMotor.IST_UNTEN
						|| zMotor.motorR == zMotor.STILL)) {
			// Ding ist Unten, soll nach oben
			zMotor.setSollHub(400);
			zMotor.setMotorState(zMotor.RAUF);
		}
		// Ding ist schon oben, aber nicht ganz
		naufGedrueckt = 0;
	}


}

// put function definitions here:
void naufINT() {
	naufGedrueckt = 1;
}

void nabINT() {
	nabGedrueckt = 1;
}

void checkTemp(void) {
	if (fT.getObenTemp() < WARN_OBEN_TEMP
			&& fT.getUntenTemp() < WARN_UNTEN_TEMP) {
		oldWarnStatus = warnStatus;
		warnStatus = NORMAL;
	}

	if (fT.getObenTemp() > WARN_OBEN_TEMP
			|| fT.getUntenTemp() > WARN_UNTEN_TEMP) {
		oldWarnStatus = warnStatus;
		warnStatus = WARNUNG;
	}

	if (fT.getObenTemp() > MAX_OBEN_TEMP
			|| fT.getUntenTemp() > MAX_UNTEN_TEMP) {
		oldWarnStatus = warnStatus;
		warnStatus = WAYTOHOT;
	}

	if (oldWarnStatus == NORMAL && warnStatus == WARNUNG) {
		warnMillis = millis();
		oldWarnStatus = WARNUNG;
	}

	if (oldWarnStatus == WARNUNG && warnStatus == WARNUNG) {
		ausgabe(1, F("R'TENTION HOT!  "));
		lcd.backlight();
		zeigTemperatur();
		sirene(1);
		zeigTemperatur();

		if (millis() - warnMillis > 20000 && zMotor.getHub() < 100) {
			zMotor.setSollHub(100);
			zMotor.setMotorState(zMotor.RAUF);
			warnMillis = millis();
		}
	}

	//es ist wieder kälter geworden
	if ((oldWarnStatus == WARNUNG || oldWarnStatus == WAYTOHOT)
			&& warnStatus == NORMAL) {
		zMotor.setSollHub(0);
		zMotor.setMotorState(zMotor.RUNTER);
		oldWarnStatus = NORMAL;
		digitalWrite(TASTER_NAB_LED_PIN, LOW);
		digitalWrite(TASTER_NAUF_LED_PIN, LOW);
		ausgabe(1, F("bassd wieda.    "));
		delay(3000);
		attachInterrupt(digitalPinToInterrupt(TASTER_NAB_PIN), nabINT, FALLING);
		attachInterrupt(digitalPinToInterrupt(TASTER_NAUF_PIN), naufINT,
		FALLING);
	}

	if (warnStatus == WAYTOHOT) {
		lcd.backlight();
		ausgabe(1, F("WAY TO HOT!     "));
		sirene2(1);
		zeigTemperatur();
		sirene2(1);
	}

	if ((oldWarnStatus == WARNUNG || oldWarnStatus == NORMAL)
			&& warnStatus == WAYTOHOT) {
		if (zMotor.getProzent() < 100) {
			zMotor.setSollHub(398);
			zMotor.setMotorState(zMotor.RAUF);
		}
		oldWarnStatus = WAYTOHOT;
		detachInterrupt(digitalPinToInterrupt(TASTER_NAB_PIN));
		detachInterrupt(digitalPinToInterrupt(TASTER_NAUF_PIN));
	}

	if (oldWarnStatus == WAYTOHOT && warnStatus == WARNUNG) {
		warnMillis = millis();
		if (zMotor.getHub() > 100) {
			zMotor.setHub(100);
			zMotor.setMotorState(zMotor.RUNTER);
		}
		oldWarnStatus = WARNUNG;
		detachInterrupt(digitalPinToInterrupt(TASTER_NAB_PIN));
		detachInterrupt(digitalPinToInterrupt(TASTER_NAUF_PIN));
	}
}

void kalibrieren(void) {
	uint32_t antwort;
	ausgabe(0, F("Taste loslassen"));
	readTaste(TASTER_NAUF_LED_PIN);

	delay(1500);

	ausgabe(0, F("Gleich warm?    "));
	ausgabe(1, F("NAUF: Ja NAB: Na"));
	delay(3000);
	uint8_t taste = 0;
	do {
		if (!digitalRead(TASTER_NAB_PIN)) {
			taste = 2;
		}

		if (!digitalRead(TASTER_NAUF_PIN)) {
			taste = 1;
		}
		delay(50);
	} while (taste == 0);

	if (taste == 1 && fT.calibrateOffset() == 0) {
		lcd.clear();
		ausgabe(0, F("Offset kalibr.  "));
		sprintf_P(lcdstring, PSTR("neu: %3d°C"), fT.getOffset());
		ausgabe(1, lcdstring);
		eeprom_write_word((uint16_t*) (START_ADDR + OFFSET_ADDR),
				fT.getOffset()); //Temp Offset
		delay(4000);
	} else {
		lcd.clear();
		ausgabe(0, F("Temperatur nicht"));
		ausgabe(1, F("kalibriert."));
		delay(2000);
	}

	ausgabe(0, F("Kalibrierung    "));
	ausgabe(1, F("etz owe drugga  "));
	antwort = zMotor.kalibriere(TASTER_NAB_PIN);
	ausgabe(0, F("sollt unt sa    "));
	ausgabe(1, F("etz drugga+haltn"));
	antwort = zMotor.kalibriere(TASTER_NAUF_PIN);
	sprintf(lcdstring, "affe: %lu ms", antwort);
	ausgabe(0, lcdstring);
	ausgabe(1, F("etz drugga+haltn"));
	antwort = zMotor.kalibriere(TASTER_NAB_PIN);
	sprintf(lcdstring, "owe: %lu ms", antwort);
	ausgabe(0, lcdstring);
	ausgabe(1, F("etz bist ferte!"));
	delay(5000);

	eeprom_write_dword((uint32_t*) (START_ADDR + NABTIME_ADDR),
			zMotor.getLaufzeitNab());
	eeprom_write_dword((uint32_t*) (START_ADDR + NAUFTIME_ADDR),
			zMotor.getLaufzeitNauf());
	eeprom_write_word((uint16_t*) (START_ADDR + HUBMS_ADDR), zMotor.getHubMs());

}

void zeigTemperatur(void) {
// lcd.createChar(0, tempObenChar);
// lcd.createChar(1, gradCChar);
// lcd.createChar(2, deltaTChar);
// lcd.createChar(3, tempUntenChar);

	sprintf(zahl, "%2d", fT.getObenTemp());
	lcd.setCursor(0, 0);
	lcd.write(0); //temp Oben Char
	lcd.setCursor(1, 0);
	lcd.print(zahl);
	lcd.setCursor(3, 0);
	lcd.write(1); // °C Char

	sprintf(zahl, "%3d", fT.getDeltaTemp());
	lcd.setCursor(5, 0);
	lcd.write(2); //delta T Char
	lcd.setCursor(6, 0);
	lcd.print(zahl);
	lcd.setCursor(9, 0);
	lcd.write(1); // °C Char

	sprintf(zahl, "%3d", fT.getUntenTemp());
	lcd.setCursor(11, 0);
	lcd.write(3); // temp Unten Char
	lcd.setCursor(12, 0);
	lcd.print(zahl);
	lcd.setCursor(15, 0);
	lcd.write(1); // °C Char

}

void sirene(uint8_t howOften) {
	for (int l = 0; l < howOften; l++) {
		digitalWrite(TASTER_NAB_LED_PIN, LOW);
		digitalWrite(TASTER_NAUF_LED_PIN, HIGH);
		for (int i = 300; i < 1000; i++) {
			tone(BUZZER_PIN, i);
			delay(1);
		}
		digitalWrite(TASTER_NAB_LED_PIN, HIGH);
		digitalWrite(TASTER_NAUF_LED_PIN, LOW);
		for (int i = 1000; i > 300; i--) {
			tone(BUZZER_PIN, i);
			delay(2);
		}
		digitalWrite(TASTER_NAB_LED_PIN, LOW);
		digitalWrite(TASTER_NAUF_LED_PIN, LOW);
	}
	noTone(BUZZER_PIN);
}

void sirene2(uint8_t howOften) {
	for (int l = 0; l < howOften; l++) {
		digitalWrite(TASTER_NAB_LED_PIN, LOW);
		digitalWrite(TASTER_NAUF_LED_PIN, HIGH);
		for (int i = 300; i < 1000; i++) {
			tone(BUZZER_PIN, i);
		}
		digitalWrite(TASTER_NAB_LED_PIN, HIGH);
		digitalWrite(TASTER_NAUF_LED_PIN, LOW);

		lcd.print(lcdstring);
		for (int i = 1000; i > 300; i--) {
			tone(BUZZER_PIN, i);
		}
		digitalWrite(TASTER_NAB_LED_PIN, LOW);
		digitalWrite(TASTER_NAUF_LED_PIN, LOW);
		noTone(BUZZER_PIN);
	}
}

void powermessung() {
// 30A ACS712 Sensor
// Nullpunkt 2,5V max 0,52V
// Vin Spannungsteiler *3, zwischen 7 und 13V misst er ziemlich ok -> wird a koa uhr.
// 66mV pro Ampere
	uint16_t rawVin, rawAin;
	int16_t vIn, aIn, tempAin;

	rawVin = analogRead(TEG_VOLTAGE_PIN);           // für mV
	rawAin = analogRead(TEG_AMPERE_PIN);            // für mV
	vIn = map(rawVin, 0, 1023, 0, 14200); // sollte mal 3 sein (0-15000), aber nicht ganz korrekt
	tempAin = 2492 - map(rawAin, 0, 1024, 0, 5000); // jetza hamma mV
	if (tempAin < 0) {
		tempAin = 0;
	}
	aIn = map(tempAin, 0, 2500 - 520, 0, 30000); // das sollten jetzt mA sein
	powerW = (aIn / 10) * (vIn / 100);           // mW!

	sprintf_P(lcdstring, PSTR("%2d.%01dV%2d.%01dA%3d.%01dW"), vIn / 1000,
			(abs(vIn % 1000) / 100), aIn / 1000, (abs(aIn % 1000) / 100),
			powerW / 1000, (abs(powerW % 1000) / 100));
	ausgabe(1, lcdstring);
}

void ausgabe(uint8_t zeile, const __FlashStringHelper *text) {
	lcd.setCursor(0, zeile);
	lcd.print(text);
}

void ausgabe(uint8_t zeile, const char *text) {
	lcd.setCursor(0, zeile);
	lcd.print(text);
}

bool readTaste(uint16_t tastenpin) {
	uint32_t tastenMillis = millis();
	bool taste = 1;
	while (taste && millis() - tastenMillis < 1000) {
		taste = digitalRead(tastenpin);
		delay(1);
	}
	return (!taste);
}

uint16_t readTastenTime(uint16_t tastenpin) {
	//uint16_t tastenTime = 0;
	uint32_t tastenMillis = millis();
	while (!digitalRead(tastenpin)) {
		delay(1);
	}
	return (millis() - tastenMillis);
}

bool readTasten(uint16_t tastenpin1, uint16_t tastenpin2) {
	uint32_t tastenMillis = millis();
	bool taste1 = 1;
	bool taste2 = 1;
	while ((taste1 || taste2) && millis() - tastenMillis < 500) {
		taste1 = !digitalRead(tastenpin1);
		taste2 = !digitalRead(tastenpin2);
		Serial.print(tastenpin1);
		Serial.print(tastenpin2);
		Serial.print(" ");
		Serial.print(millis() - tastenMillis);
		Serial.print(taste1);
		Serial.println(taste2);
	}
	return ((taste1 || taste2));
}

// #colt sein ding
void colt(int buzzerPin) {

	tone(buzzerPin, 139);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 156);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(526);
	noTone(buzzerPin);

	tone(buzzerPin, 208);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(526);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(353);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(703);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(526);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(526);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(526);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(526);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 466);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(174);
	noTone(buzzerPin);

	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(526);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(526);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 208);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(703);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 330);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(526);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(526);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(526);
	noTone(buzzerPin);

	tone(buzzerPin, 208);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 466);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(526);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 554);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 587);
	delay(526);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 587);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 554);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(526);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(526);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 330);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 330);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 233);
	delay(262);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(350);
	noTone(buzzerPin);

	tone(buzzerPin, 139);
	delay(174);
	noTone(buzzerPin);
	/*
	 tone(buzzerPin, 156);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 185);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 185);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 185);
	 delay(526);
	 noTone(buzzerPin);

	 tone(buzzerPin, 208);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 247);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 247);
	 delay(526);
	 noTone(buzzerPin);

	 tone(buzzerPin, 247);
	 delay(526);
	 noTone(buzzerPin);

	 tone(buzzerPin, 247);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 277);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 277);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 277);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 247);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 311);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 370);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 370);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 415);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 415);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 415);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 415);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 415);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 370);
	 delay(526);
	 noTone(buzzerPin);

	 tone(buzzerPin, 370);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 247);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 277);
	 delay(526);
	 noTone(buzzerPin);

	 tone(buzzerPin, 415);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 370);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 370);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 415);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 415);
	 delay(526);
	 noTone(buzzerPin);

	 tone(buzzerPin, 415);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 370);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 370);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 415);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 370);
	 delay(526);
	 noTone(buzzerPin);

	 tone(buzzerPin, 415);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 415);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 415);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 370);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 370);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 247);
	 delay(526);
	 noTone(buzzerPin);

	 tone(buzzerPin, 311);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 247);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 247);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 247);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 277);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 277);
	 delay(526);
	 noTone(buzzerPin);

	 tone(buzzerPin, 277);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 277);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 349);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 494);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 466);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 415);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 370);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 415);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 466);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 311);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 370);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 415);
	 delay(526);
	 noTone(buzzerPin);

	 tone(buzzerPin, 370);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 415);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 370);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 415);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 494);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 494);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 587);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 587);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 587);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 587);
	 delay(526);
	 noTone(buzzerPin);

	 tone(buzzerPin, 494);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 415);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 494);
	 delay(526);
	 noTone(buzzerPin);

	 tone(buzzerPin, 415);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 494);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 415);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 370);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 247);
	 delay(526);
	 noTone(buzzerPin);

	 tone(buzzerPin, 277);
	 delay(174);
	 noTone(buzzerPin);

	 tone(buzzerPin, 311);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 330);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 330);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 277);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 233);
	 delay(350);
	 noTone(buzzerPin);

	 tone(buzzerPin, 247);
	 delay(350);
	 noTone(buzzerPin);
	 */
}

void colt2(int buzzerPin) {

	tone(buzzerPin, 139);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 156);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 208);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 466);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 208);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 330);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 208);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 466);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 554);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 587);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 587);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 554);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 330);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 330);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 233);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 139);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 156);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 185);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 208);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 349);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 466);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 466);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 587);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 587);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 587);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 587);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 415);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 311);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 330);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 330);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 277);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 233);
	delay(174);
	noTone(buzzerPin);

	tone(buzzerPin, 247);
	delay(174);
	noTone(buzzerPin);
}

void snuggles(int buzzerPin) {

	tone(buzzerPin, 294);
	delay(290);
	noTone(buzzerPin);

	tone(buzzerPin, 330);
	delay(145);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(282);
	noTone(buzzerPin);

	tone(buzzerPin, 392);
	delay(588);
	noTone(buzzerPin);

	tone(buzzerPin, 294);
	delay(290);
	noTone(buzzerPin);

	tone(buzzerPin, 330);
	delay(145);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(290);
	noTone(buzzerPin);

	tone(buzzerPin, 392);
	delay(435);
	noTone(buzzerPin);

	tone(buzzerPin, 440);
	delay(145);
	noTone(buzzerPin);

	tone(buzzerPin, 392);
	delay(435);
	noTone(buzzerPin);

	tone(buzzerPin, 262);
	delay(1739);
	noTone(buzzerPin);

	tone(buzzerPin, 294);
	delay(290);
	noTone(buzzerPin);

	tone(buzzerPin, 330);
	delay(145);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(282);
	noTone(buzzerPin);

	tone(buzzerPin, 392);
	delay(588);
	noTone(buzzerPin);

	tone(buzzerPin, 294);
	delay(290);
	noTone(buzzerPin);

	tone(buzzerPin, 330);
	delay(145);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(290);
	noTone(buzzerPin);

	tone(buzzerPin, 392);
	delay(435);
	noTone(buzzerPin);

	tone(buzzerPin, 440);
	delay(145);
	noTone(buzzerPin);

	tone(buzzerPin, 494);
	delay(435);
	noTone(buzzerPin);

	tone(buzzerPin, 659);
	delay(1739);
	noTone(buzzerPin);

	tone(buzzerPin, 587);
	delay(870);
	noTone(buzzerPin);

	tone(buzzerPin, 294);
	delay(290);
	noTone(buzzerPin);

	tone(buzzerPin, 330);
	delay(145);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(282);
	noTone(buzzerPin);

	tone(buzzerPin, 392);
	delay(588);
	noTone(buzzerPin);

	tone(buzzerPin, 294);
	delay(290);
	noTone(buzzerPin);

	tone(buzzerPin, 330);
	delay(145);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(290);
	noTone(buzzerPin);

	tone(buzzerPin, 440);
	delay(435);
	noTone(buzzerPin);

	tone(buzzerPin, 392);
	delay(145);
	noTone(buzzerPin);

	tone(buzzerPin, 370);
	delay(435);
	noTone(buzzerPin);

	tone(buzzerPin, 392);
	delay(870);
	noTone(buzzerPin);
}
