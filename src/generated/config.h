#ifndef CONFIG_H
#define CONFIG_H

struct configData
{
	char projectName[32];
	char projectVersion[32];
	volatile uint16_t ResetGrund;
	volatile bool ShowFilePage;
	volatile bool ShowFileFirmwarePage;
	volatile bool ShowWebFirmwarePage;
	volatile bool ShowDiagnosePage;
	char language[3];
	volatile bool UseDMADC;
	volatile float Ch1EinLevel;
	volatile float Ch2EinLevel;
	volatile float mVperAmp;
	volatile float AmpOffset1;
	volatile float AmpOffset2;
	volatile uint16_t FirebaseUpdateIntervall;
	char PrivateKeyFirst[4];
	volatile bool ConnectToCloud;
	volatile bool UseStream;
	volatile bool UseWatchdog;
	volatile bool ImmerLogoutBeiError;
	char FirmwareURL[60];
	volatile uint16_t Messagelevel;
	volatile float Ch1WarmwasserLevelMin1;
	volatile float Ch1WarmwasserLevelMax1;
	volatile float Ch1WarmwasserLevelMin2;
	volatile bool EnableAusfallDetektor;
	char StartFenster[20];
	char EndFenster[20];
	volatile uint16_t MaxPausenzeit;
	volatile uint16_t DrainageWarnmeldungZyklen;
	volatile uint16_t DrainageWarnmeldungZeit;
	volatile bool SMLEnable;
	volatile uint16_t PowerDifferenz;
	volatile uint16_t EnablePeakFilter;
	volatile bool SolarEnableGartenhaus;
	char IPAddressShellyGartenhaus[60];
	volatile bool SolarEnableGarage;
	char IPAddressShellyGarage[60];
	volatile uint16_t Abfrageintervall;
};

extern uint32_t configVersion;
extern const configData defaults;

#endif