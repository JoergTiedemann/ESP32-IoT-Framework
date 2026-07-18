#ifndef WIFI_H
#define WIFI_H

#include <Arduino.h>
#include <DNSServer.h>
#include <memory>
#include "StringArray.h"
#include "FirebaseJson.h"

class WifiManager
{

private:
    DNSServer *dnsServer;
    String ssid;
    String pass;
    IPAddress ip;
    IPAddress gw;
    IPAddress sub;
    IPAddress dns;
    bool reconnect = false;
    bool inCaptivePortal = false;
    char const *captivePortalName;
    unsigned long timeout = 60000;
    void startCaptivePortal(char const *apName);
    void stopCaptivePortal();
    void connectNewWifi(String newSSID, String newPass);    
    void storeToEEPROM();
    int8_t waitForConnectResult(unsigned long timeoutLengthMs);
    bool isIPAddressSet(IPAddress ip);
    std::function<void()> _forgetwificallback;
    std::function<void()> _newwificallback;    

public : 
    FirebaseJsonArray m_LastWifiInfo;
    volatile bool m_bNetworkScanRunning=false;
    volatile bool m_bNetworkScanDone=false;
    int m_ScanResult = 0;
    void begin(char const *apName, unsigned long newTimeout = 60000);
    void loop();
    void forget();
    bool isCaptivePortal();
    String SSID();
    long RSSI();
    void setNewWifi(String newSSID, String newPass);
    void setNewWifi(String newSSID, String newPass, String newIp, String newSub, String newGw, String newDns);
    void forgetWiFiFunctionCallback( std::function<void()> func );
    void newWiFiFunctionCallback( std::function<void()> func );
    String StartNetworkscan();
    void GetScanString(FirebaseJsonArray *pwifiInfo);
     ~WifiManager()
	{
	}
};

extern WifiManager WiFiManager;

#endif


