Kabel 5x2,5mm²:
 1		12V+
 2		GND
 3		TEG+ IN
 4		TEG+ OUT
 Erdung		GND TEG
 
Kabel 10x1mm²
 1		DS18B20 Tempfühler 5V+
 2		DS18B20 Tempfühler GND
 3		DS18B20 Tempfühler Data
 4		Thermocouple +
 5		Thermocouple -
 6		Motor +  NC
 7		Motor +                                        
 8		Motor - 
 9		Motor - NC 
 Erdung		NC
  
Arduino NANO:
	usb
D13	SCK	MAX6675K    D12	MISO -> Max6675K SO
3V3			        D11~MOSI
REF			        D10~SS   -> Piezo  done
A0	Input ACS712 d	D9~	Motor Enable  done
A1	Input TEG V	d   D8	L298N Input4  done
A2	D16 DS18b20     D7	L298N Input3  done
A3			        D6~	LED Taster nauf PWM  done
A4	SDA Display d	D5~	LED Taster nab  PWM  done
A5	SCL Display	d	D4  -> MAX6675 CS select
A6  nur A/D         D3~ LED Taster nab INT  done
A7  nur A/D         D2  LED Taster nauf INT  done
5V   done           GND  done
RST                 RESET
GND                 D0  RX
VIN                 D1  TX


Taster: 
Entprellt mit Doppel-NAND Gatter 74HC00N, 1k auf 5V
-le old school-
12V taster-Leds mit Optokoppler und 330 ohm am uC

Voltage sensing:
Spannungsteiler 19:80
