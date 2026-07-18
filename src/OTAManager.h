#ifndef OTAMAN_H
#define OTAMAN_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>


class COTAManager
{

private:

public : 
    HTTPClient client;
    int totalLength;       //total size of firmware
    int currentLength; //current size of written firmware
    bool m_bDoUpdate;

    void begin();
    void DoOTAUpdate(String strHOST);
    void updateFirmware(uint8_t *data, size_t len);

    COTAManager()
	{
	}

    ~COTAManager()
	{
	}

};

extern COTAManager OTAManager;

#endif


