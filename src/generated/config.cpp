#include <Arduino.h>
#include "config.h"

uint32_t configVersion = 186963199; //generated identifier to compare config with EEPROM

const configData defaults PROGMEM =
{
	"Heizungs-Monitor",
	"V6.7",
	0,
	false,
	false,
	true,
	true,
	"de",
	true,
	0.6,
	0.5,
	39.6,
	-0.1,
	-0.1,
	25000,
	"",
	false,
	false,
	false,
	false,
	"http://www.bierhoehle.de/Heizung.bin",
	2,
	1.3,
	1.6,
	2.2,
	true,
	"07:30:00",
	"22:00:00",
	20,
	3,
	400,
	false,
	50,
	4,
	false,
	"http://192.168.178.118",
	false,
	"http://192.168.178.123",
	8
};