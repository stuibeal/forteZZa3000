//
//    FILE: forteZZaTemp.cpp
//  AUTHOR: Rob Tillaart, Alfred Stuiber


#include "forteZZaTemp.h"
#include "Arduino.h"
#include "SPI.h"
#include "OneWire.h"

forteZZaTemp::forteZZaTemp(uint8_t selectOneWire, uint8_t select, __SPI_CLASS__ * mySPI)
{
  _select = select;
  _selectOneWire = selectOneWire;
  _miso   = 255;
  _clock  = 255;
  _mySPI  = mySPI;
  _hwSPI  = true;
  _addressFound = false;
  _resolution   = 9;
  _config       = DS18B20_CLEAR;
  _oneWire = nullptr;
  _tempOben = 0;
  _tempDelta = 0;
  _tempUnten = 0;
  _lastTimeRead = 0;
  _rawData = 0;
  _offset = 0;
  _SPIspeed= 0;
  _temperature = 0;
  _status = 0;
}

bool forteZZaTemp::begin(uint8_t retries)
{
  _lastTimeRead = 0;
  _offset       = 0;
  _status       = STATUS_NOREAD;
  _temperature  = DS18B20_NO_TEMPERATURE;
  _rawData      = 0;

  setSPIspeed(1000000);

  pinMode(_select, OUTPUT);
  digitalWrite(_select, HIGH);

  _oneWire = new OneWire (_selectOneWire);
  _config = DS18B20_CLEAR;
  if (isConnected(retries))
  {
    _setResolution();
  }
  return _addressFound;
}

// Temperaturfühler anfragen
void forteZZaTemp::request(void){
    //DS18B20 anfragen
	if (isConversionComplete())
    {
    	int16_t temp = getTempC();
		if (temp != 0) {
    		_tempOben = temp;
    	}
    }
	requestTemperatures ();
    //ThermoCouple anfragen
    readUnten();
    
}

bool forteZZaTemp::calibrateOffset(void){
  int16_t tempDS18 = 1000;
  requestTemperatures();
  while(!isConversionComplete()) {
	  delay(1);
  }
  delay(1000);
  tempDS18 = getTempC();
  if (tempDS18 > 0) {
    _tempOben = tempDS18;
  } else {return 1;}
  
  if (readUnten() != STATUS_NO_COMMUNICATION) {
    _offset = _tempOben - _tempUnten;
  } else {return 1;}
return (0);
}

uint8_t forteZZaTemp::readUnten()
{
  //  return value of _read()  page 5 datasheet
  //  BITS       DESCRIPTION
  //  ------------------------------
  //       00    three state ?
  //       01    device ID ?
  //       02    INPUT OPEN
  //  03 - 14    TEMPERATURE (RAW)
  //       15    SIGN
	uint16_t value = _read();

	  //  needs a pull up on MISO pin to work properly!
	  if (value == 0xFFFF)
	  {
	    _status = STATUS_NO_COMMUNICATION;
	    return _status;
	  }

	  _lastTimeRead = millis();

	  //  process status bit 2
	  _status = value & 0x04;

	   value >>= 3;

	  //  process temperature bits
	  _tempUnten = (value & 0x1FFF) * 0.25;
	  //  dummy negative flag set ?
	  //  if (value & 0x2000)
	  return _status;
	}


uint16_t forteZZaTemp::getDeltaTemp(void) 
{
    _tempDelta = 0;
    if (_tempOben < (_tempUnten + _offset)) {
        _tempDelta = (_tempUnten + _offset) - _tempOben;
    }
    return _tempDelta;
}


void forteZZaTemp::setSPIspeed(uint32_t speed)
{
  _SPIspeed = speed;
  _spi_settings = SPISettings(_SPIspeed, MSBFIRST, SPI_MODE0);
};



//////////////
// DS18B20

bool forteZZaTemp::isConnected(uint8_t retries)
{
  _addressFound = false;
  for (uint8_t rtr = retries; (rtr > 0) && (_addressFound == false); rtr--)
  {
    _oneWire->reset();
    _oneWire->reset_search();
    _deviceAddress[0] = 0x00;
    _oneWire->search(_deviceAddress);
    _addressFound = _deviceAddress[0] != 0x00 &&
                _oneWire->crc8(_deviceAddress, 7) == _deviceAddress[7];
  }
  return _addressFound;
}

void forteZZaTemp::requestTemperatures(void)
{
  _oneWire->reset();
  _oneWire->skip();
  _oneWire->write(STARTCONVO, 0);
  
}

bool forteZZaTemp::isConversionComplete(void)
{
  return (_oneWire->read_bit() == 1);
}


int16_t forteZZaTemp::getTempC(void)
{
  ScratchPad scratchPad;
  if (isConnected(3) == false)
  {
    return DEVICE_DISCONNECTED;
  }

  if (_config & DS18B20_CRC)
  {
    readScratchPad(scratchPad, 9);
    if (_oneWire->crc8(scratchPad, 8) != scratchPad[SCRATCHPAD_CRC])
    {
      return DEVICE_CRC_ERROR;
    }
  }
  else
  {
    readScratchPad(scratchPad, 2);
  }
  int16_t rawTemperature = (((int16_t)scratchPad[TEMP_MSB]) << 8) | scratchPad[TEMP_LSB];
  int16_t temp = 0.0625 * rawTemperature;
  if (temp < -55)
  {
    return DEVICE_DISCONNECTED;
  }
  return temp;
}


bool forteZZaTemp::getAddress(uint8_t* buf)
{
  if (_addressFound)
  {
    for (uint8_t i = 0; i < 8; i++)
    {
      buf[i] = _deviceAddress[i];
    }
  }
  return _addressFound;
}


bool forteZZaTemp::setResolution(uint8_t resolution)
{
  if (isConnected())
  {
    _resolution = resolution;
    _setResolution();
  }
  return _addressFound;
}


uint8_t forteZZaTemp::getResolution()
{
  return _resolution;
}


void forteZZaTemp::setConfig(uint8_t config)
{
  _config = config;
}


uint8_t forteZZaTemp::getConfig()
{
  return _config;
}


//////////////////////////////////////////////////
//
//  PRIVATE
//
uint32_t forteZZaTemp::_read(void)
{
  _rawData = 0;
  //  DATA TRANSFER
    _mySPI->beginTransaction(_spi_settings);
    //  must be after mySPI->beginTransaction() - STM32 (#14 MAX31855_RT)
    digitalWrite(_select, LOW);
    _rawData = _mySPI->transfer(0);
    _rawData <<= 8;
    _rawData += _mySPI->transfer(0);
    digitalWrite(_select, HIGH);
    _mySPI->endTransaction();
  return _rawData;
}


void forteZZaTemp::readScratchPad(uint8_t *scratchPad, uint8_t fields)
{
  _oneWire->reset();
  _oneWire->select(_deviceAddress);
  _oneWire->write(READSCRATCH);

  for (uint8_t i = 0; i < fields; i++)
  {
    scratchPad[i] = _oneWire->read();
  }
  _oneWire->reset();
}


void forteZZaTemp::_setResolution()
{
  uint8_t res;
  switch (_resolution)
  {
    case 12: res = TEMP_12_BIT;  break;
    case 11: res = TEMP_11_BIT;  break;
    case 10: res = TEMP_10_BIT;  break;
    //  lowest as default as we do only integer math.
    default: res = TEMP_9_BIT;   break;
  }

  _oneWire->reset();
  _oneWire->select(_deviceAddress);
  _oneWire->write(WRITESCRATCH);
  //  two dummy values for LOW & HIGH ALARM
  _oneWire->write(0);
  _oneWire->write(100);
  _oneWire->write(res);
  _oneWire->reset();
}




//  -- END OF FILE --

