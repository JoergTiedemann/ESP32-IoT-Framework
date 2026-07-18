//inspired by https://github.com/tzapu/WiFiManager but 
//- with more flexibility to add your own web server setup
//= state machine for changing wifi settings on the fly

#include <WiFi.h>
#include <esp_wifi.h>

#include "WiFiManager.h"
#include "configManager.h"

//create global object
WifiManager WiFiManager;

String getBssidString(const uint8_t* bssid)
{
    String result;
    for (int i = 0; i < 6; ++i) {
        if (i > 0) {
            result += ":";
        }
        if (bssid[i] < 0x10) {
            result += "0";
        }
        result += String(bssid[i], HEX);
    }
    result.toUpperCase();
    return result;
}

String getAuthModeName(wifi_auth_mode_t authMode)
{
    switch (authMode) {
        case WIFI_AUTH_OPEN:
            return "open";
        case WIFI_AUTH_WEP:
            return "WEP";
        case WIFI_AUTH_WPA_PSK:
            return "WPA/PSK";
        case WIFI_AUTH_WPA2_PSK:
            return "WPA2/PSK";
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WPA/WPA2/PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE:
            return "WPA2/Enterprise";
        case WIFI_AUTH_WPA3_PSK:
            return "WPA3/PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK:
            return "WPA2/WPA3/PSK";
        case WIFI_AUTH_WAPI_PSK:
            return "WAPI/PSK";
        default: {
            String result = "unknown(";
            result += authMode;
            result += ")";
            return result;
        }
    }
}

// Evenmthant handler for WiFi events, specifically for scan done event
void WiFiEvent(WiFiEvent_t event) {
    if (event == ARDUINO_EVENT_WIFI_SCAN_DONE) {
        WiFiManager.m_bNetworkScanRunning = false;
        WiFiManager.m_bNetworkScanDone = true;
    }
}

//function to call in setup
void WifiManager::begin(char const *apName, unsigned long newTimeout)
{
    captivePortalName = apName;
    timeout = newTimeout;
    WiFi.onEvent(WiFiEvent);
    WiFi.mode(WIFI_STA);
    WiFi.persistent(true);
    WiFi.setAutoReconnect(true);
    
    //set static IP if entered
    ip = IPAddress(configManager.internal.ip);
    gw = IPAddress(configManager.internal.gw);
    sub = IPAddress(configManager.internal.sub);
    dns = IPAddress(configManager.internal.dns);

    if (isIPAddressSet(ip) || isIPAddressSet(gw) || isIPAddressSet(sub) || isIPAddressSet(dns))
    {
        Serial.println(PSTR("Using static IP"));
        WiFi.config(ip, gw, sub, dns);
    }

    // ESP32 PITA: workaround configured persisted SSID not being restored to WiFi.SSID() 
    // See https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html
    // See https://github.com/espressif/arduino-esp32/issues/548#
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);
    String configSsid = F(conf.sta.ssid);
    String configPsk = F(conf.sta.password);

    Serial.println("configSsid=");
    Serial.println(configSsid);

    if (configSsid == "")
    {
        Serial.println(PSTR("Configured SSID is empty."));
        // Timeout runter setzen
        timeout = 2000;
    }
    else
    {
        WiFi.disconnect(false);

        Serial.print(PSTR("WiFi.begin(), WifiManager::begin(), SSID: "));
        Serial.print(configSsid);

        WiFi.begin();

    }

    if (waitForConnectResult(timeout) == WL_CONNECTED)
    {
        //connected
        Serial.print(PSTR("Connected to stored WiFi details"));
        Serial.print(PSTR(", localIP: "));
        Serial.println(WiFi.localIP());
    }
    else
    {
        //captive portal
        Serial.print(PSTR("Timed out waiting for connect. Starting captive portal. SSID: "));
        Serial.println(WiFi.SSID());
        startCaptivePortal(captivePortalName);
    }
}

//Upgraded default waitForConnectResult function to incorporate WL_NO_SSID_AVAIL, fixes issue #122
int8_t WifiManager::waitForConnectResult(unsigned long timeoutLengthMs) {
#ifdef ESP32
    // 1 (WIFI_MODE_STA) and 3 (WIFI_MODE_APSTA) have STA enabled
    if((WiFiGenericClass::getMode() & WIFI_MODE_STA) == 0) {
        Serial.print(PSTR("STA mode not enabled, returning WL_DISCONNECTED."));
        return WL_DISCONNECTED;
    }

    // Wait to become connected, or timeout expiration.  Bail if clock rollover detected.
    unsigned long now = millis();
    unsigned long start = now;
    unsigned long timeout = now + timeoutLengthMs;
    int loopCount = 0;
    while((now = millis()) < timeout && now >= start) {
        delay(1);
        wl_status_t wifiStatus = WiFi.status();
        if(wifiStatus != WL_DISCONNECTED &&     // Disconnected from a networ
            wifiStatus != WL_NO_SSID_AVAIL &&   // When no SSID are available
            wifiStatus != WL_IDLE_STATUS)       // Temporary status assigned when WiFi.begin() called
        {
            Serial.print(PSTR("WiFi.status()="));
            Serial.println(wifiStatus);
            return wifiStatus;
        }
        loopCount++;
    }

    Serial.print(PSTR("Loop count="));
    Serial.print(loopCount);

#elif defined(ESP8266)
    // 1 (WIFI_MODE_STA) and 3 (WIFI_MODE_APSTA) have STA enabled
    if((wifi_get_opmode() & 1) == 0) {
        return WL_DISCONNECTED;
    }
    using esp8266::polledTimeout::oneShot;
    oneShot timeout(timeoutLengthMs); // number of milliseconds to wait before returning timeout error
    while(!timeout) {
        yield();
        if(WiFi.status() != WL_DISCONNECTED && WiFi.status() != WL_NO_SSID_AVAIL) {
            return WiFi.status();
        }
    }
#endif

    return -1; // -1 indicates timeout
}

//function to forget current WiFi details and start a captive portal
void WifiManager::forget()
{ 
    inCaptivePortal = true; // damit der IP Adressencheck nicht zuschlaegt
    WiFi.disconnect();
    // esp_wifi_restore();
    startCaptivePortal(captivePortalName);

    //remove IP address from EEPROM
    ip = IPAddress();
    sub = IPAddress();
    gw = IPAddress();
    dns = IPAddress();

    //make EEPROM empty
    storeToEEPROM();

    if ( _forgetwificallback != NULL) {
        _forgetwificallback();
    } 

    Serial.println(PSTR("Requested to forget WiFi. Started Captive portal."));
}

//function to request a connection to new WiFi credentials
void WifiManager::setNewWifi(String newSSID, String newPass)
{    
    ssid = newSSID;
    pass = newPass;
    ip = IPAddress();
    sub = IPAddress();
    gw = IPAddress();
    dns = IPAddress();
    reconnect = true;
}

//function to request a connection to new WiFi credentials
void WifiManager::setNewWifi(String newSSID, String newPass, String newIp, String newSub, String newGw, String newDns)
{
    ssid = newSSID;
    pass = newPass;
    ip.fromString(newIp);
    sub.fromString(newSub);
    gw.fromString(newGw);
    dns.fromString(newDns);

    reconnect = true;
}

//function to connect to new WiFi credentials
void WifiManager::connectNewWifi(String newSSID, String newPass)
{
    delay(1000);

    //set static IP or zeros if undefined    
    WiFi.config(ip, gw, sub, dns);

    bool hasNetworkMismatch = 
        // operator uint32_t() const
        ip != IPAddress(configManager.internal.ip) || 
        dns != IPAddress(configManager.internal.dns);

    //fix for auto connect racing issue
    if (!(WiFi.status() == WL_CONNECTED && (WiFi.SSID() == newSSID)) || hasNetworkMismatch)
    {          
        //trying to fix connection in progress hanging
        WiFi.disconnect(false);

        //store old data in case new network is wrong
        // ESP32 PITA workaround configured persisted SSID not being restored to WiFi.SSID() 
        wifi_config_t conf;
        esp_wifi_get_config(WIFI_IF_STA, &conf);
        String oldSSID = F(conf.sta.ssid);
        String oldPSK = F(conf.sta.password);


        Serial.print(PSTR("WiFi.begin, connectNewWifi"));
        Serial.print(", SSID: ");
        Serial.println(newSSID.c_str());

        WiFi.begin(newSSID.c_str(), newPass.c_str(), 0, NULL, true);
        delay(2000);

        // TODO:P0 Implement ESP32 version,
        // ESP32 doesn't have timeout param for WiFi.waitForConnectResult, times out after 10s, so either live with that or create wrapper...
        if (WiFi.waitForConnectResult() != WL_CONNECTED)
        {
            Serial.println(PSTR("New connection unsuccessful"));
            if (!inCaptivePortal)
            {
                Serial.print(PSTR("WiFi.begin, !inCaptivePortal"));

                WiFi.begin(
                    oldSSID.c_str(), // ssid
                    oldPSK.c_str(),  // passphrase
                    0,               // channel
                    NULL,            // BSSID / MAC of AP
                    true);           // connect

                if (WiFi.waitForConnectResult() != WL_CONNECTED)
                {
                    Serial.println(PSTR("Reconnection failed too"));
                    startCaptivePortal(captivePortalName);
                }
                else 
                {
                    Serial.println(PSTR("Reconnection successful"));
                    Serial.println(WiFi.localIP());
                }
            }
        }
        else
        {
            if (inCaptivePortal)
            {
                stopCaptivePortal();
            }

            Serial.println(PSTR("New connection successful"));
            Serial.println(WiFi.localIP());

            //store IP address in EEProm
            storeToEEPROM();

            if ( _newwificallback != NULL) {
                _newwificallback();
            }

        }
    }
}

//function to start the captive portal
void WifiManager::startCaptivePortal(char const *apName)
{
    WiFi.persistent(false);
    // disconnect sta, start ap
    WiFi.disconnect(); //  this alone is not enough to stop the autoconnecter
    WiFi.mode(WIFI_AP_STA);//anstatt WIFI_AP damit der Wlan Scan weiterhin funktioniert
    WiFi.persistent(true);

    WiFi.softAP(apName);

    dnsServer = new DNSServer();

    /* Setup the DNS server redirecting all the domains to the apIP */
    dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer->start(53, "*", WiFi.softAPIP());

    // TODO: Consider "4.3.2.1" or something else that's easier to remember...
    Serial.println(PSTR("Opened a captive portal"));
    Serial.println(PSTR("192.168.4.1"));
    inCaptivePortal = true;

}

//function to stop the captive portal
void WifiManager::stopCaptivePortal()
{    
    WiFi.mode(WIFI_STA);
    delete dnsServer;

    inCaptivePortal = false;
}

void  WifiManager::forgetWiFiFunctionCallback( std::function<void()> func ) {
  _forgetwificallback = func;
}

void WifiManager::newWiFiFunctionCallback( std::function<void()> func ) {
  _newwificallback = func;
}

//return captive portal state
bool WifiManager::isCaptivePortal()
{
    return inCaptivePortal;
}

//return current SSID
String WifiManager::SSID()
{    
    return WiFi.SSID();
}

long WifiManager::RSSI()
{    
    return WiFi.RSSI();
}


//captive portal loop
void WifiManager::loop()
{
    if (inCaptivePortal)
    {
        //captive portal loop
        dnsServer->processNextRequest();
    }

    if (reconnect)
    {
        connectNewWifi(ssid, pass);
        reconnect = false;
    }
    // InternNetworkscan();
}
 

String WifiManager::StartNetworkscan()
{
    // Serial.printf("StartNetworkscan\n");
    wifi_scan_config_t cfg = {
            .ssid = NULL,
            .bssid = NULL,
            .channel = 0,
            .show_hidden = true,
            .scan_type = WIFI_SCAN_TYPE_ACTIVE,
            .scan_time = {.active = {150, 150}}
        };

    esp_err_t err = esp_wifi_scan_start(&cfg, false); // false = async
    Serial.printf("esp_wifi_scan_start result:%d\n",err);
    m_LastWifiInfo.clear();
    if (err == ESP_OK) {
        m_bNetworkScanRunning = true;
        m_bNetworkScanDone = false;
        return "Wifi-Scan gestartet";
    } else {
        return "Wifi-Scan start fehlerhaft";
    }
}


void WifiManager::GetScanString(FirebaseJsonArray *pwifiInfo)
{
    if (!m_bNetworkScanDone) {
        m_LastWifiInfo.clear();
        m_LastWifiInfo.add(String("Wifi-Scan noch nicht beendet"));
        return;
    }
    uint16_t apCount = 0;
    String str ="";
    FirebaseJsonData data;
    esp_wifi_scan_get_ap_num(&apCount);
    if ((apCount == 0) && (m_LastWifiInfo.size() > 0)) {
        // wir haben keinen neuen Scan und nehmen daher die alten Daten
        for (int i = 0; i < m_LastWifiInfo.size(); i++) {
            m_LastWifiInfo.get(data, i );
            str = data.to<String>();
            pwifiInfo->add(str);
        }
        return;
    }

    wifi_ap_record_t *list = (wifi_ap_record_t*)malloc(sizeof(wifi_ap_record_t) * apCount);
    esp_wifi_scan_get_ap_records(&apCount, list);

    for (int i = 0; i < apCount; i++) {
        str = "SSID:";
        str += String((char*)list[i].ssid);
        str += "( BSSID:";
        str += getBssidString(list[i].bssid);
        str += ") RSSI:";
        str += list[i].rssi;
        str += "dB Channel:";
        str += list[i].primary;
        str += " Sicherheit:";
        str += getAuthModeName(list[i].authmode);
        // Serial.printf("Index:%d Result:%s\n",i,str.c_str());
        m_LastWifiInfo.add(str);
    }
    free(list);
    for (int i = 0; i < m_LastWifiInfo.size(); i++) {
        m_LastWifiInfo.get(data, i );
        str = data.to<String>();
        pwifiInfo->add(str);
    }

    m_bNetworkScanRunning = false;
    return;
}

//update IP address in EEPROM
void WifiManager::storeToEEPROM()
{
    configManager.internal.ip = ip;
    configManager.internal.gw = gw;
    configManager.internal.sub = sub;
    configManager.internal.dns = dns;
        
    configManager.save();
}

bool WifiManager::isIPAddressSet(IPAddress ip)
{
#if defined(ESP8266)
    return ip.isSet();
#else
    return ip.toString() == "0.0.0.0";
#endif
}

