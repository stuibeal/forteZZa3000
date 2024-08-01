#include "motor.h"
#include "EEPROM.h"

motor::motor() {
	_prozent = 0;
	_laufzeitNab = 29450;
	_laufzeitNauf = 29557; // 13V
	_msProMmHub = 68;
	_kaliStatus = 0;
	_mySerial = nullptr;
	_laufzeit = 0;
	_hub = 0;
	_oldMillis = millis();
	_sollProzent = 0;
	_sollHub = 0;
	motorS = STOP;
	motorR = STILL;
}

motor::~motor() {
	// nothing to do here
}

void motor::begin(HardwareSerial *mySerial) {
	_mySerial = mySerial;
	pinMode(MOTOR_ENABLE_PIN, OUTPUT);
	pinMode(MOTORIN_PIN, OUTPUT);
	pinMode(MOTOROUT_PIN, OUTPUT);
	pinMode(TASTER_NAUF_LED_PIN, OUTPUT);
	pinMode(TASTER_NAB_LED_PIN, OUTPUT);
}

void motor::check() {
	switch (motorS) {
	case STOP:
		digitalWrite(MOTOROUT_PIN, LOW);
		digitalWrite(MOTORIN_PIN, LOW);
		analogWrite(MOTOR_ENABLE_PIN, 0);
		analogWrite(TASTER_NAB_LED_PIN, LOW);
		analogWrite(TASTER_NAUF_LED_PIN, LOW);
		if (_prozent == 0) {
			motorR = IST_UNTEN;
		} else if (_prozent == 100) {
			motorR = IST_OBEN;
		} else {
			motorR = STILL;
		}
		break;
	case RAUF:
		digitalWrite(MOTOROUT_PIN, HIGH);
		sanftAnfahren(TASTER_NAUF_LED_PIN);
		motorS = RUNNING;
		motorR = NAUF;
		_oldMillis = millis();
		break;
	case RUNNING:
		_laufzeit = millis() - _oldMillis;
		if (motorR == NAB) {
			_hub = 400 - SANFT_HUB - (_laufzeit / _msProMmHub);
			if (_hub <= _sollHub + SANFT_HUB) {
				sanftAuslaufen(TASTER_NAB_LED_PIN);
				motorS = STOP;
				_hub = 0;
				_prozent = 0;
			}
		}
		if (motorR == NAUF) {
			_hub = SANFT_HUB + (_laufzeit / _msProMmHub);
			if (_hub >= _sollHub - SANFT_HUB) { //
				sanftAuslaufen(TASTER_NAUF_LED_PIN);
				motorS = STOP;
				_hub = 400;
				_prozent = 100;
			}
		}

		_prozent = (int16_t) _hub / 4;
		break;
	case RUNTER:
		digitalWrite(MOTORIN_PIN, HIGH);
		sanftAnfahren(TASTER_NAB_LED_PIN);
		motorS = RUNNING;
		motorR = NAB;
		_oldMillis = millis();
		break;
	}
}

void motor::sanftAnfahren(uint16_t ledpin) {
	for (int i = 100; i < 255; i++) {
		analogWrite(MOTOR_ENABLE_PIN, i);
		analogWrite(ledpin, i);
		delay(10);
	}
	analogWrite(MOTOR_ENABLE_PIN, 255);
}

void motor::sanftAuslaufen(uint16_t ledpin) {
	for (int i = 255; i > 60; i--) {
		analogWrite(MOTOR_ENABLE_PIN, i);
		analogWrite(ledpin, i);
		delay(10);
	}
	analogWrite(MOTOR_ENABLE_PIN, 0);
}

/* Laufzeiten:
 for schleife 60-255, delay 5: 979 ms, hub: 20,6mm
 for schleife 60-255, delay 2x10: 2x1955 ms, hub: 42,26mm
 for schleife 60-255, delay 20: 3904 ms, hub: 53mm
 hub gesamt: 398 mm
 laufzeit 1955 ms ohne pwm: 28,76mm, ms pro mm: 68

 ABER: Alles bei 13,0 V!!!!!
 */

uint32_t motor::kalibriere(uint16_t taster) {

	uint32_t oldMillis = 0;
	switch (_kaliStatus) {
	case GANZRUNTER:

		digitalWrite(MOTOROUT_PIN, LOW);
		digitalWrite(MOTORIN_PIN, HIGH);
		digitalWrite(TASTER_NAB_LED_PIN, HIGH);
		while (digitalRead(taster)) {
			delay(1);
		}
		while (!digitalRead(taster)) {
			digitalWrite(MOTOR_ENABLE_PIN, HIGH);
		}
		digitalWrite(MOTORIN_PIN, LOW);
		digitalWrite(MOTOR_ENABLE_PIN, LOW);
		digitalWrite(TASTER_NAB_LED_PIN, LOW);
		_kaliStatus++;
		return 0;

	case RAUFKALIBRIEREN:
		digitalWrite(TASTER_NAUF_LED_PIN, HIGH);
		digitalWrite(MOTOROUT_PIN, HIGH);
		while (digitalRead(taster)) {
			delay(1);
		}
		oldMillis = millis();
		while (!digitalRead(taster)) {
			digitalWrite(MOTOR_ENABLE_PIN, HIGH);
		}
		_laufzeitNauf = millis() - oldMillis;
		digitalWrite(MOTOROUT_PIN, LOW);
		digitalWrite(MOTOR_ENABLE_PIN, LOW);
		digitalWrite(TASTER_NAUF_LED_PIN, LOW);
		_kaliStatus++;
		_msProMmHub = _laufzeitNauf / 398;
		return _laufzeitNauf;

	case RUNTERKALIBRIEREN:
		digitalWrite(TASTER_NAB_LED_PIN, HIGH);
		digitalWrite(MOTORIN_PIN, HIGH);
		while (digitalRead(taster)) {
			delay(1);
		}
		oldMillis = millis();
		while (!digitalRead(taster)) {
			digitalWrite(MOTOR_ENABLE_PIN, HIGH);
		}
		_laufzeitNab = millis() - oldMillis;
		digitalWrite(MOTORIN_PIN, LOW);
		digitalWrite(MOTOR_ENABLE_PIN, LOW);
		digitalWrite(TASTER_NAB_LED_PIN, LOW);
		_kaliStatus++;

		return _laufzeitNab;
	default:
		return 0;
	}
}

// EOF
