#ifndef DASH_H
#define DASH_H

struct dashboardData
{
	uint16_t aktuelleLaufzeit1;
	uint16_t aktuelleLaufzeit2;
	bool Heizung;
	bool Warmwasser;
	bool Drainage;
	int16_t PowerSum;
	int16_t PowerL1;
	int16_t PowerL2;
	int16_t PowerL3;
	float AktPowerSolarGartenhaus;
	float AktReturnedGartenhaus;
	float AktPowerSolarGarage;
	float AktReturnedGarage;
	float InputCurrent1;
	float InputCurrent2;
	uint16_t DisplayDiagnostic;
	bool Testschalter1;
	bool Testschalter2;
	bool MessageTest;
	uint16_t AnzeigeinMin;
};

#endif