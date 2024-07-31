/* Z-Gesellschaft Motor Library
*  Zum Ansteuern des 400mm Linearmotors
*
*/
#include "Arduino.h"
#include <stdint.h>
#include "common.h"


class motor 
{
    public: motor(); //Constructor
    virtual 
    ~motor(); //Destructor
    void begin(HardwareSerial *mySerial);
    void check(void);
    void nauf(uint16_t prozent);
    void nab(uint16_t prozent);
    uint32_t kalibriere(uint16_t taster);
    inline uint16_t getProzent(void) {return _prozent;};
    inline uint16_t getHub(void) {return _hub;};
    inline uint32_t getLaufzeitNauf(void) {return _laufzeitNauf;}
    inline uint32_t getLaufzeitNab(void) {return _laufzeitNab;}
    inline void setLaufzeitNauf(uint32_t laufzeit) {_laufzeitNauf = laufzeit;}
    inline void setLaufzeitNab(uint32_t laufzeit) {_laufzeitNab = laufzeit;}
    inline uint16_t getHubMs(void) {return _msProMmHub;}
    inline void setHubMs(uint16_t hubms) {_msProMmHub = hubms;}

    enum motorRichtung
    {
        NAUF, NAB
    };

    enum kaliState
    {
      GANZRUNTER, RAUFKALIBRIEREN, RUNTERKALIBRIEREN, KALIBRIERT
    };
uint32_t _laufzeitNauf;
    uint32_t _laufzeitNab;

    private:
    int16_t _hub;
    int16_t _prozent;
    uint16_t _kaliStatus;
    uint32_t _oldMillis;
    HardwareSerial  *_mySerial;
    uint16_t _msProMmHub;
    uint32_t _laufzeit;
};

//EOF