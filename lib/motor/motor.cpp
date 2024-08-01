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
	_oldHub = 0;
	_hubAktuell = 0;
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
		if (_hub > 399) {
			motorR = IST_OBEN;
		} else if (_hub < 1) {
			motorR = IST_UNTEN;
		} else {
			motorR = STILL;
		}
		break;
	case RAUF:
		Serial.println(F("Starte rauf"));
		digitalWrite(MOTORIN_PIN, LOW);
		digitalWrite(MOTOROUT_PIN, HIGH);
		motorS = RUNNING;
		motorR = NAUF;
		_oldMillis = millis();
		_oldHub = _hub;
		Serial.print(_laufzeitNauf);
		Serial.println(_msProMmHub);

		_msProMmHub = _laufzeitNauf / 398;

		Serial.print(_laufzeitNauf);
		Serial.println(_msProMmHub);
		break;
	case RUNNING:
		_laufzeit = millis() - _oldMillis;
		if (motorR == NAB) {
			_hubAktuell = _laufzeit / _msProMmHub;
			_hub = _oldHub - _hubAktuell;
			if (_hubAktuell < SANFT_HUB + 2) {
				sanftAnfahren(TASTER_NAB_LED_PIN, _hubAktuell);
			}
			if (_hub < _sollHub + SANFT_HUB + 2) {
				uint16_t restHub = _hub - _sollHub;
				sanftAuslaufen(TASTER_NAB_LED_PIN, restHub);
			}
			if (_hub <= _sollHub) {
				motorS = STOP;
				_hub = 0;
				_prozent = 0;
			}
		}
		if (motorR == NAUF) {
			_hubAktuell = _laufzeit / _msProMmHub;
			_hub = _oldHub + _hubAktuell;
			if (_hubAktuell < SANFT_HUB + 2) {
				sanftAnfahren(TASTER_NAUF_LED_PIN, _hubAktuell);
			}
			if (_hub > _sollHub - SANFT_HUB - 2) {
				uint16_t restHub = _sollHub - _hub;
				sanftAuslaufen(TASTER_NAUF_LED_PIN, restHub);
			}
			if (_hub >= _sollHub) {
				motorS = STOP;
				_hub = 400;
				_prozent = 100;
			}
		}

		_prozent = (int16_t) _hub / 4;
		break;
	case RUNTER:
		digitalWrite(MOTOROUT_PIN, LOW);
		digitalWrite(MOTORIN_PIN, HIGH);
		motorS = RUNNING;
		motorR = NAB;
		_oldMillis = millis();
		_oldHub = _hub;
		_msProMmHub = _laufzeitNab /398;
		Serial.print(_laufzeitNauf);
		Serial.println(_msProMmHub);

		break;
	case RAUFEXTRA:
		digitalWrite(MOTORIN_PIN, LOW);
		digitalWrite(MOTOROUT_PIN, HIGH);
		analogWrite(MOTOR_ENABLE_PIN, 255);
		delay(1500);
		motorR = IST_OBEN;
		motorS = STOP;
		digitalWrite(MOTOROUT_PIN, LOW);
		analogWrite(MOTOR_ENABLE_PIN,0);
		break;
	case RUNTEREXTRA:
		digitalWrite(MOTOROUT_PIN, LOW);
		digitalWrite(MOTORIN_PIN, HIGH);
		analogWrite(MOTOR_ENABLE_PIN, 255);
		delay(1500);
		digitalWrite(MOTORIN_PIN, LOW);
		analogWrite(MOTOR_ENABLE_PIN, 0);
		motorR = IST_UNTEN;
		motorS = STOP;
		break;

	}
}

void motor::sanftAnfahren(uint16_t ledpin, uint16_t restHub) {
	uint8_t anfangPwm = 255 - SANFT_HUB * 6;
	uint16_t pwm = anfangPwm + restHub * 6;
	if (pwm > 255) {
		pwm = 255;
	}
	analogWrite(MOTOR_ENABLE_PIN, (int) pwm);
	analogWrite(ledpin, (int) pwm);
	_mySerial->print(pwm);
}

void motor::sanftAuslaufen(uint16_t ledpin, uint16_t restHub) {
	uint8_t anfangPwm = 255 - SANFT_HUB * 6;
	uint16_t pwm = anfangPwm + restHub * 6;
	if (pwm > 255) {
		pwm = 255;
	}
	analogWrite(MOTOR_ENABLE_PIN, (int) pwm);
	analogWrite(ledpin, (int) pwm);
	_mySerial->print(pwm);
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
		break;
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
		break;
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
		break;
	default:
		return 0;
		break;
	}
}

// EOF
