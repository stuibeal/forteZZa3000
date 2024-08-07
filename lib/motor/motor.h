/* Z-Gesellschaft Motor Library
 *  Zum Ansteuern des 400mm Linearmotors
 *
 */
#include "Arduino.h"
#include <stdint.h>
#include "common.h"

#define SANFT_HUB 20 //mm

class motor {
public:
	motor(); //Constructor
	virtual
	~motor(); //Destructor
	void begin(HardwareSerial *mySerial);
	void check(void);
	void sanftAnfahren(uint16_t ledpin, uint16_t restHub);
	void sanftAuslaufen(uint16_t ledpin, uint16_t restHub);
	uint32_t kalibriere(uint16_t taster);
	inline uint16_t getProzent(void) {
		return _prozent;
	}
	inline uint16_t getHub(void) {
		return _hub;
	}
	inline void setHub(uint16_t setHub) {
		_hub = setHub;
	}

	inline uint32_t getLaufzeitNauf(void) {
		return _laufzeitNauf;
	}
	inline uint32_t getLaufzeitNab(void) {
		return _laufzeitNab;
	}
	inline void setLaufzeitNauf(uint32_t laufzeit) {
		_laufzeitNauf = laufzeit;
	}
	inline void setLaufzeitNab(uint32_t laufzeit) {
		_laufzeitNab = laufzeit;
	}
	inline void setSollHub(int16_t mm) {
		_sollHub = mm;
	}
	inline uint16_t getHubMs(void) {
		return _msProMmHub;
	}
	inline void setHubMs(uint16_t hubms) {
		_msProMmHub = hubms;
	}
	inline void setMotorState(uint8_t state) {
		motorS = state;
	}
	inline uint8_t getMotorState(void) {
		return motorS;
	}

	enum motorRichtung {
		NAUF, NAB, STILL, IST_UNTEN, IST_OBEN
	};

	enum motorState {
		STOP, RAUF, RUNNING, RUNTER, RAUFEXTRA, RUNTEREXTRA
	};

	enum kaliState {
		GANZRUNTER, RAUFKALIBRIEREN, RUNTERKALIBRIEREN, KALIBRIERT
	};
	uint32_t _laufzeitNauf;
	uint32_t _laufzeitNab;
	uint16_t _msProMmHub;

	motorState motorS;
	motorRichtung motorR;

private:
	int16_t _hub;
	int16_t _prozent;
	uint16_t _kaliStatus;
	uint32_t _oldMillis;
	HardwareSerial *_mySerial;
	uint32_t _laufzeit;
	int16_t _sollProzent;
	int16_t _sollHub;
	uint16_t _hubAktuell;
	uint16_t _oldHub;
};

//EOF
