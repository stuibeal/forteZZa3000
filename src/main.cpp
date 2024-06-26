/* Z-Gesellschaft
 *  forteZZa3000 (R)
 *  Sicherheitsregulator für ofenbasierte Thermoelektrische Generatoren
 *
 */
#include "main.h"

// LIBS
LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display
forteZZaTemp fT(DS18B20_PIN, THERMOCOUPLE_PIN, &SPI);
motor zMotor;

void setup()
{
  // put your setup code here, to run once:
  // start serial port
  Serial.begin(9600);
  Serial.println("forteZZa3000");

  // Temperatursensoren
  SPI.begin();
  fT.begin();
  fT.setSPIspeed(2000);
  fT.setOffset(-5); // ThermoCouple Offset
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
  lcd.setCursor(0, 0);
  lcd.print("forteZZa3000");
  lcd.setCursor(0, 1);
  lcd.print("Z-Gesellschaft");

  // Sirene mal testen
  sirene(1);

  // Dr. Snuggles huldigen
  // snuggles(BUZZER_PIN);
  // Motor kalibrieren
  if (!digitalRead(TASTER_NAB_PIN) || !digitalRead(TASTER_NAUF_PIN))
  {
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
}

void loop()
{
  if (nabPressMillis != 0 || naufPressMillis != 0)
  {
    Serial.print(naufPressMillis);
    Serial.print(" ");
    Serial.println(nabPressMillis);
  }

  if (naufPressMillis > 0)
  {
    // bei einmaligen drücken wenn er oben ist -> Display an
    lcd.backlight();
    backLightMillis = millis();

    // Doppelklickzeit überprüfen, wenn über 2s Klick löschen
    if (millis() - doppelKlickTime > 2000)
    {
      nabPressMillis = 0;
      naufPressMillis = 0;
      doppelKlickTime = millis();
    }

    // Wenn der Motor auf 100% ist aber nicht ganz oben
    if (naufPressMillis > 1 && zMotor.getProzent() == 100)
    {
      lcd.setCursor(0, 1);
      lcd.print("NAUF extra 3000 ");
      naufPressMillis = 0;
      zMotor.nauf(20);
      lcd.setCursor(0, 1);
      sprintf(lcdstring, "%3d %% %3d mm   ", zMotor.getProzent(), zMotor.getHub());
      lcd.print(lcdstring);
      delay(3000); // Warten damit der Horst das lesen kann
      naufPressMillis = 0;
    }

    // nur wenn der Taster noch gedrückt ist (int spinnt teilweise)
    if (!digitalRead(TASTER_NAUF_PIN))
    {

      // Wenn der Motor nicht ganz oben ist -> bewegen
      if (zMotor.getProzent() < 100)
      {
        lcd.setCursor(0, 1);
        lcd.print("NAUF            ");
        zMotor.nauf(100);
        lcd.setCursor(0, 1);
        sprintf(lcdstring, "%3d %% %3d mm  ", zMotor.getProzent(), zMotor.getHub());
        lcd.print(lcdstring);
        naufPressMillis = 0;
        delay(3000);
        if (zMotor.getProzent() == 100)
        {
          colt(BUZZER_PIN);
        }
      }
    }
  }

  if (nabPressMillis > 0)
  {
    // bei einmaligen drücken wenn er unten ist -> Display an
    lcd.backlight();
    backLightMillis = millis();

    // Doppelklickzeit überprüfen, wenn über 2s Klick löschen
    if (millis() - doppelKlickTime > 2000)
    {
      nabPressMillis = 0;
      naufPressMillis = 0;
      doppelKlickTime = millis();
    }

    // Wenn der Motor auf 0% ist aber nicht ganz unten
    if (nabPressMillis > 1 && zMotor.getProzent() == 0)
    {
      lcd.setCursor(0, 1);
      lcd.print("NAB extra 3000  ");
      nabPressMillis = 0;
      zMotor.nab(20);
      lcd.setCursor(0, 1);
      sprintf(lcdstring, "%3d %% %3d mm  ", zMotor.getProzent(), zMotor.getHub());
      lcd.print(lcdstring);
      delay(3000); // Warten damit der Horst das lesen kann
      nabPressMillis = 0;
    }

    // nur wenn der Taster noch gedrückt ist (int spinnt teilweise)
    if (!digitalRead(TASTER_NAB_PIN))
    {

      // Wenn der Motor nicht ganz unten ist -> bewegen
      if (!zMotor.getProzent() == 0)
      {
        lcd.setCursor(0, 1);
        lcd.print("NAB             ");
        zMotor.nab(100);
        lcd.setCursor(0, 1);
        sprintf(lcdstring, "%3d %% %3d mm  ", zMotor.getProzent(), zMotor.getHub());
        lcd.print(lcdstring);
        nabPressMillis = 0;
        delay(3000);
        if (zMotor.getProzent() == 0)
        {
          snuggles(BUZZER_PIN);
        }
      }
    }
  }

  if (millis() - oldMillis > 500)
  {

    oldMillis = millis();
    fT.request();
    delay(20);
    zeigTemperatur();
    if (millis() - backLightMillis > BACKLIGHT_MILLIS)
    {
      lcd.noBacklight();
    }
    checkTemp();
    powermessung();
  }
}

// put function definitions here:
void naufINT()
{
  naufPressMillis++;
  doppelKlickTime = millis();
}

void nabINT()
{
  nabPressMillis++;
  doppelKlickTime = millis();
}

void checkTemp(void)
{
  if (fT.getObenTemp() < WARN_OBEN_TEMP && fT.getUntenTemp() < WARN_UNTEN_TEMP)
  {
    oldWarnStatus = warnStatus;
    warnStatus = NORMAL;
    Serial.print("WarnStatusNOR:");
    Serial.print(warnStatus);
    Serial.println(oldWarnStatus);
  }

  if (fT.getObenTemp() > WARN_OBEN_TEMP || fT.getUntenTemp() > WARN_UNTEN_TEMP)
  {
    oldWarnStatus = warnStatus;
    warnStatus = WARNUNG;
    Serial.print("WarnStatusWAR:");
    Serial.print(warnStatus);
    Serial.println(oldWarnStatus);
  }

  if (fT.getObenTemp() > MAX_OBEN_TEMP || fT.getUntenTemp() > MAX_UNTEN_TEMP)
  {
    oldWarnStatus = warnStatus;
    warnStatus = WAYTOHOT;
    Serial.print("WarnStatusMAX:");
    Serial.print(warnStatus);
    Serial.println(oldWarnStatus);
  }

  if (oldWarnStatus == NORMAL && warnStatus == WARNUNG)
  {
    warnMillis = millis();
    oldWarnStatus = WARNUNG;
  }

  if (oldWarnStatus == WARNUNG && warnStatus == WARNUNG)
  {
    lcd.setCursor(0, 1);
    lcd.print("R'TENTION HOT!  ");
    lcd.backlight();
    zeigTemperatur();
    sirene(1);
    zeigTemperatur();
    Serial.print("warnMillis: ");
    Serial.println(warnMillis);

    if (millis() - warnMillis > 20000 && zMotor.getProzent() < 10)
    {
      zMotor.nauf(20
      );
      warnMillis = millis();
    }
  }

  if ((oldWarnStatus == WARNUNG || oldWarnStatus == WAYTOHOT) && warnStatus == NORMAL)
  {
    if (zMotor.getProzent() > 0)
    {
      zMotor.nab(100);
    }

    oldWarnStatus = NORMAL;
    digitalWrite(TASTER_NAB_LED_PIN, LOW);
    digitalWrite(TASTER_NAUF_LED_PIN, LOW);
    lcd.setCursor(0, 1);
    lcd.print("bassd wieda.    ");
    delay(3000);
    attachInterrupt(digitalPinToInterrupt(TASTER_NAB_PIN), nabINT, FALLING);
    attachInterrupt(digitalPinToInterrupt(TASTER_NAUF_PIN), naufINT, FALLING);
  }

  if (warnStatus == WAYTOHOT)
  {
    lcd.setCursor(0, 1);
    lcd.print("WAY TO HOT!     ");
    lcd.backlight();
    sirene2(1);
  }
  if (oldWarnStatus == WARNUNG && warnStatus == WAYTOHOT && zMotor.getProzent() < 100)
  {
    zMotor.nauf(100);
    oldWarnStatus = WAYTOHOT;
    detachInterrupt(digitalPinToInterrupt(TASTER_NAB_PIN));
    detachInterrupt(digitalPinToInterrupt(TASTER_NAUF_PIN));
  }

  if (oldWarnStatus == WAYTOHOT && warnStatus == WARNUNG && zMotor.getProzent() > 10)
  {
    warnMillis = millis();
    zMotor.nab(85);
    oldWarnStatus = WARNUNG;
    detachInterrupt(digitalPinToInterrupt(TASTER_NAB_PIN));
    detachInterrupt(digitalPinToInterrupt(TASTER_NAUF_PIN));
  }
}

void kalibrieren(void)
{
  uint32_t antwort;
  lcd.setCursor(0, 0);
  lcd.print("Kalibrierung    ");
  lcd.setCursor(0, 1);
  lcd.print("etz owe drugga  ");
  antwort = zMotor.kalibriere(TASTER_NAB_PIN);
  lcd.setCursor(0, 0);
  lcd.print("sollt unt sa    ");
  lcd.setCursor(0, 1);
  lcd.print("etz drugga+haltn");
  antwort = zMotor.kalibriere(TASTER_NAUF_PIN);
  sprintf(lcdstring, "affe: %lu ms", antwort);
  lcd.setCursor(0, 0);
  lcd.print(lcdstring);
  lcd.setCursor(0, 1);
  lcd.println("etz drugga+haltn");
  antwort = zMotor.kalibriere(TASTER_NAB_PIN);
  sprintf(lcdstring, "owe: %lu ms", antwort);
  lcd.setCursor(0, 0);
  lcd.print(lcdstring);
  lcd.setCursor(0, 1);
  lcd.print("etz bist ferte!");
  delay(5000);
}

void zeigTemperatur(void)
{
  lcd.setCursor(1, 0);
  sprintf(zahl, "%2d", fT.getObenTemp());
  lcd.print(zahl);

  lcd.setCursor(12, 0);
  sprintf(zahl, "%3d", fT.getUntenTemp());
  lcd.print(zahl);

  lcd.setCursor(6, 0);
  sprintf(zahl, "%3d", fT.getDeltaTemp());
  lcd.print(zahl);
}

void sirene(uint8_t howOften)
{
  for (int l = 0; l < howOften; l++)
    digitalWrite(TASTER_NAB_LED_PIN, LOW);
  digitalWrite(TASTER_NAUF_LED_PIN, HIGH);
  {
    for (int i = 300; i < 1000; i++)
    {
      tone(BUZZER_PIN, i);
      delay(1);
    }
    digitalWrite(TASTER_NAB_LED_PIN, HIGH);
    digitalWrite(TASTER_NAUF_LED_PIN, LOW);
    for (int i = 1000; i > 300; i--)
    {
      tone(BUZZER_PIN, i);
      delay(2);
    }
    digitalWrite(TASTER_NAB_LED_PIN, LOW);
    digitalWrite(TASTER_NAUF_LED_PIN, LOW);
  }
  noTone(BUZZER_PIN);
}

void sirene2(uint8_t howOften)
{
  for (int l = 0; l < howOften; l++)
    digitalWrite(TASTER_NAB_LED_PIN, LOW);
  digitalWrite(TASTER_NAUF_LED_PIN, HIGH);
  {
    for (int i = 300; i < 1000; i++)
      tone(BUZZER_PIN, i);
  }
  digitalWrite(TASTER_NAB_LED_PIN, HIGH);
  digitalWrite(TASTER_NAUF_LED_PIN, LOW);

  lcd.print(lcdstring);
  for (int i = 1000; i > 300; i--)
  {
    tone(BUZZER_PIN, i);
  }
  digitalWrite(TASTER_NAB_LED_PIN, LOW);
  digitalWrite(TASTER_NAUF_LED_PIN, LOW);
  noTone(BUZZER_PIN);
}

void powermessung()
{
  // 30A ACS712 Sensor
  // Nullpunkt 2,5V max 0,52V
  // Vin Spannungsteiler *3, zwischen 7 und 13V misst er ziemlich ok -> wird a koa uhr.
  // 66mV pro Ampere
  uint16_t rawVin, rawAin;
  int16_t vIn, aIn, tempAin;
  uint16_t powerW;

  rawVin = analogRead(TEG_VOLTAGE_PIN);           // für mV
  rawAin = analogRead(TEG_AMPERE_PIN);            // für mV
  vIn = map(rawVin, 0, 1023, 0, 14200);           // sollte mal 3 sein (0-15000), aber nicht ganz korrekt
  tempAin = 2492 - map(rawAin, 0, 1024, 0, 5000); // jetza hamma mV
  if (tempAin < 0)
  {
    tempAin = 0;
  }
  aIn = map(tempAin, 0, 2500 - 520, 0, 30000); // das sollten jetzt mA sein
  powerW = (aIn / 10) * (vIn / 100);           // mW!

  lcd.setCursor(0, 1);
  sprintf(lcdstring, "%2d.%01dV%2d.%01dA%3d.%01dW",
          vIn / 1000,
          (abs(vIn % 1000) / 100),
          aIn / 1000,
          (abs(aIn % 1000) / 100),
          powerW / 1000,
          (abs(powerW % 1000) / 100));
  lcd.print(lcdstring);
}

// #colt sein ding
void colt(int buzzerPin)
{

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

void colt2(int buzzerPin)
{

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

void snuggles(int buzzerPin)
{

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
