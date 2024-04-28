#include "motor.h"

motor::motor()
{
    _prozent = 0;
    _laufzeitNab = 29450;
    _laufzeitNauf = 29557; // 13V
    _msProMmHub = 68;
    _kaliStatus = 0;
    _mySerial = nullptr;
}

motor::~motor()
{
    // nothing to do here
}

void motor::begin(HardwareSerial *mySerial)
{
    _mySerial = mySerial;
    pinMode(MOTOR_ENABLE_PIN, OUTPUT);
    pinMode(MOTORIN_PIN, OUTPUT);
    pinMode(MOTOROUT_PIN, OUTPUT);
    pinMode(TASTER_NAUF_LED_PIN, OUTPUT);
    pinMode(TASTER_NAB_LED_PIN, OUTPUT);
}

void motor::check()
{
}

/* Laufzeiten:
   for schleife 60-255, delay 5: 979 ms, hub: 20,6mm
   for schleife 60-255, delay 2x10: 2x1955 ms, hub: 42,26mm
   for schleife 60-255, delay 20: 3904 ms, hub: 53mm
   hub gesamt: 398 mm
   laufzeit 1955 ms ohne pwm: 28,76mm, ms pro mm: 68

   ABER: Alles bei 13,0 V!!!!!
*/

void motor::nauf(uint16_t prozent)
{
    // Out Pin on
    digitalWrite(MOTOROUT_PIN, HIGH);

    // langsam anfahren
    for (int i = 60; i < 255; i++)
    {
        analogWrite(MOTOR_ENABLE_PIN, i);
        analogWrite(TASTER_NAUF_LED_PIN, i);
        delay(10);
    }
    // das bringt normal 21mm hub (~)
    _hub += 21;

    // wenn der Taster gehalten wurde solange fahren wie gehalten wird
    if (!digitalRead(TASTER_NAUF_PIN))
    {
        prozent = 12; // nur damit was dasteht
        _oldMillis = millis();
        while (!digitalRead(TASTER_NAUF_PIN))
        {
            delay(1);
        }
        _laufzeit = millis() - _oldMillis;
        _mySerial->println(_laufzeit);
    }
    // ansonsten nach dem Prozentwert richten
    else
    {
        uint16_t resthub = (prozent * 398 / 100) - 42;
        _mySerial->print(resthub);
        _mySerial->print(" ");
        uint32_t motdelay = resthub * _msProMmHub;
        if (motdelay > 0)
        {
            delay(motdelay);
            _laufzeit = motdelay;
        }
        _mySerial->print(_hub);
        _mySerial->print(" ");
    }
    _hub += (_laufzeit / _msProMmHub);
    _mySerial->print(_hub);
    _mySerial->println(" ");


    if (prozent == 100)
    {
        for (int i = 255; i > 20; i--)
        {
            analogWrite(MOTOR_ENABLE_PIN, i);
            analogWrite(TASTER_NAUF_LED_PIN, i);
            delay(50); // dann fährt der auch ganz zu weil länger und so
        }
        _prozent = 100;
        _hub = 398;
    }
    else
    {
        for (int i = 255; i > 60; i--)
        {
            analogWrite(MOTOR_ENABLE_PIN, i);
            analogWrite(TASTER_NAUF_LED_PIN, i);
            delay(10);
        }
        _hub += 21;
        _mySerial->print("hub:");
        _mySerial->print(_hub);
        _prozent = (_hub * 100) / 398;
        _mySerial->print("prozent");
        _mySerial->println(_prozent);
        
    }

    if (_hub > 398 || _prozent > 100)
    {
        _prozent = 100;
        _hub = 398;
    }

    digitalWrite(MOTOROUT_PIN, LOW);
    digitalWrite(TASTER_NAUF_LED_PIN, LOW);
}

void motor::nab(uint16_t prozent)
{
       // In Pin on
    digitalWrite(MOTORIN_PIN, HIGH);

    // langsam anfahren
    for (int i = 60; i < 255; i++)
    {
        analogWrite(MOTOR_ENABLE_PIN, i);
        analogWrite(TASTER_NAB_LED_PIN, i);
        delay(10);
    }
    // das bringt normal 21mm hub (~)
    _hub -= 21;

    // wenn der Taster gehalten wurde solange fahren wie gehalten wird
    if (!digitalRead(TASTER_NAB_PIN))
    {
        prozent = 12; // nur damit was dasteht
        _oldMillis = millis();
        while (!digitalRead(TASTER_NAB_PIN))
        {
            delay(1);
        }
        _laufzeit = millis() - _oldMillis;
    }

    // ansonsten nach dem Prozentwert richten
    else
    {
        uint16_t resthub = (prozent * 398 / 100) - 42;
        _mySerial->print(resthub);
        _mySerial->print(" ");
        uint32_t motdelay = resthub * _msProMmHub;
        if (motdelay > 0)
        {
            delay(motdelay);
            _laufzeit = motdelay;
        }
    }
    _hub -= (_laufzeit / _msProMmHub);

    if (prozent == 100)
    {
        for (int i = 255; i > 20; i--)
        {
            analogWrite(MOTOR_ENABLE_PIN, i);
            analogWrite(TASTER_NAB_LED_PIN, i);
            delay(50); // dann fährt der auch ganz weil länger und so wenn <12V
        }
        _prozent = 0;
        _hub = 0;
    }
    else
    {
        for (int i = 255; i > 60; i--)
        {
            analogWrite(MOTOR_ENABLE_PIN, i);
            analogWrite(TASTER_NAB_LED_PIN, i);
            delay(10);
        }
        _hub -= 21;
        _prozent = (_hub * 100) / 398;
    }

    if (_hub < 0 || _prozent < 0)
    {
        _prozent = 0;
        _hub = 0;
    }

    digitalWrite(MOTORIN_PIN, LOW);
    digitalWrite(TASTER_NAB_LED_PIN, LOW);
}

uint32_t motor::kalibriere(uint16_t taster)
{

    uint32_t oldMillis = 0;
    switch (_kaliStatus)
    {
    case GANZRUNTER:

        digitalWrite(MOTOROUT_PIN, LOW);
        digitalWrite(MOTORIN_PIN, HIGH);
        digitalWrite(TASTER_NAB_LED_PIN, HIGH);
        while (digitalRead(taster))
        {
            delay(1);
        }
        while (!digitalRead(taster))
        {
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
        while (digitalRead(taster))
        {
            delay(1);
        }
        oldMillis = millis();
        while (!digitalRead(taster))
        {
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
        while (digitalRead(taster))
        {
            delay(1);
        }
        oldMillis = millis();
        while (!digitalRead(taster))
        {
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