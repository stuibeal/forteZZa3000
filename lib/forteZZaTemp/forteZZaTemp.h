#pragma once
/* Z forteZZa3000 Temperaturlib
   gibt Temperaturen nur als INT aus.
   Das meiste geklaut vom Rob Tilaart seiner MAX6675 und DS18B20 Arduino LIB
*/

#include "Arduino.h"
#include "SPI.h"
#include "OneWire.h"

//  Error Code DS18B20
#define DEVICE_DISCONNECTED     -127
#define DEVICE_CRC_ERROR        -128

//  configuration codes DS18B20
#define DS18B20_CLEAR           0x00
#define DS18B20_CRC             0x01
typedef uint8_t DeviceAddress[8];
typedef uint8_t ScratchPad[9];


#ifndef __SPI_CLASS__
  #define __SPI_CLASS__   SPIClass
#endif


#define DS18B20_NO_TEMPERATURE            -999

//  STATE constants returned by read()
//  TODO check
#define STATUS_OK                         0x00
#define STATUS_ERROR                      0x04
#define STATUS_NOREAD                     0x80
#define STATUS_NO_COMMUNICATION           0x81

//  Thermocouples working is based upon Seebeck effect.
//  Different TC have a different Seebeck Coefficient  (µV/°C)
//  See http://www.analog.com/library/analogDialogue/archives/44-10/thermocouple.html
//
//  K_TC == 41.276 µV/°C

//DS18B20

//  OneWire commands
#define STARTCONVO              0x44
#define READSCRATCH             0xBE
#define WRITESCRATCH            0x4E


//  Scratchpad locations
#define TEMP_LSB                 0
#define TEMP_MSB                 1
#define HIGH_ALARM_TEMP          2
#define LOW_ALARM_TEMP           3
#define CONFIGURATION            4
#define INTERNAL_BYTE            5
#define COUNT_REMAIN             6
#define COUNT_PER_C              7
#define SCRATCHPAD_CRC           8


//  Device resolution
#define TEMP_9_BIT              0x1F    //   9 bit
#define TEMP_10_BIT             0x3F    //  10 bit
#define TEMP_11_BIT             0x5F    //  11 bit
#define TEMP_12_BIT             0x7F    //  12 bit



class forteZZaTemp
{
public:   forteZZaTemp(uint8_t oneWirePin, uint8_t select, __SPI_CLASS__ * mySPI);
  //       returns state - bit field: 0 = STATUS_OK
  bool      begin(uint8_t retries = 3);
  
  //        Temperatursensoren anfragen
  void      request(void);
  
  uint8_t  readUnten();
  inline int16_t  getUntenTemp(void)  { return _tempUnten + _offset; };
  inline int16_t  getObenTemp(void)   {return _tempOben;};
  uint16_t getDeltaTemp(void);
  inline uint8_t  getStatus(void) const { return _status; };

  //       use offset to calibrate the TC.
  void     setOffset(const int16_t  t)   { _offset = t; };
  int16_t  getOffset() const           { return _offset; };

  uint32_t lastRead()    { return _lastTimeRead; };
  uint16_t getRawData()  { return _rawData;};

  //       speed in Hz
  void     setSPIspeed(uint32_t speed);
  uint32_t getSPIspeed() { return _SPIspeed; };
  

//DS18b20
  bool      isConnected(uint8_t retries = 3);

  void      requestTemperatures(void);
  bool      isConversionComplete(void);
  int16_t   getTempC(void);

  bool      getAddress(uint8_t * buf);

  bool      setResolution(uint8_t resolution = 9);
  uint8_t   getResolution();  //  returns cached value

  void      setConfig(uint8_t config);
  uint8_t   getConfig();




private:
  uint32_t _read();
  uint8_t  _status;
  int16_t  _temperature;
  int16_t _tempOben;
  int16_t _tempUnten;
  uint16_t _tempDelta;
  int16_t _offset;
  uint32_t _lastTimeRead;
  uint16_t _rawData;
  bool     _hwSPI;

  uint8_t  _clock;
  uint8_t  _miso;
  uint8_t  _select;

  uint32_t    _SPIspeed;
  __SPI_CLASS__ * _mySPI;
  SPISettings     _spi_settings;
  
  //DS18B20
  OneWire       *_oneWire;
  uint8_t       _selectOneWire;  
  void          readScratchPad(uint8_t *, uint8_t);
  void          _setResolution();
  DeviceAddress _deviceAddress;
  bool          _addressFound;
  uint8_t       _resolution;
  uint8_t       _config;
};


//  -- END OF FILE --

