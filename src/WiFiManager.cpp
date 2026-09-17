//inspired by https://github.com/tzapu/WiFiManager but 
//- with more flexibility to add your own web server setup
//= state machine for changing wifi settings on the fly

#include <WiFi.h>
#include <esp_wifi.h>

#include "DiagManager.h"
#include "WiFiManager.h"
#include "configManager.h"
#include "NVSManager.h"

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

bool parseMac(const char* macStr, uint8_t mac[6]) {
    int values[6];
    if (sscanf(macStr, "%x:%x:%x:%x:%x:%x",
               &values[0], &values[1], &values[2],
               &values[3], &values[4], &values[5]) != 6) {
        return false;
    }
    for (int i = 0; i < 6; i++) mac[i] = (uint8_t)values[i];
    return true;
}
bool tryPreferredBssid(const String& preferredMacStr)
{
    if (preferredMacStr.length() < 17) {
        Serial.println("Preferred MAC ungültig");
        return false;
    }

    // String → char*
    const char* preferredMac = preferredMacStr.c_str();

    uint8_t targetBssid[6];
    if (!parseMac(preferredMac, targetBssid)) {
        Serial.println("Preferred MAC parse Fehler");
        return false;
    }

    // SSID + Passwort aus ESP-IDF holen
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);

    const char* ssid = (const char*)conf.sta.ssid;
    const char* pass = (const char*)conf.sta.password;

    Serial.printf("Stored SSID: %s\n", ssid);
    Serial.printf("Stored PASS: %s\n", pass);

    // Scan
    Serial.println("ESP-IDF Scan...");
    wifi_scan_config_t scanConf = {};
    scanConf.show_hidden = true;

    esp_wifi_scan_start(&scanConf, true);

    uint16_t apCount = 0;
    esp_wifi_scan_get_ap_num(&apCount);

    if (apCount == 0) {
        Serial.println("Scan: keine APs gefunden");
        return false;
    }

    wifi_ap_record_t *list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * apCount);
    esp_wifi_scan_get_ap_records(&apCount, list);

    Serial.printf("Gefundene APs: %d\n", apCount);

    bool found = false;
    int channel = 0;

    for (int i = 0; i < apCount; i++) {

        Serial.printf("SSID: %s | BSSID: %02X:%02X:%02X:%02X:%02X:%02X | Kanal: %d | RSSI: %d\n",
            list[i].ssid,
            list[i].bssid[0], list[i].bssid[1], list[i].bssid[2],
            list[i].bssid[3], list[i].bssid[4], list[i].bssid[5],
            list[i].primary,
            list[i].rssi
        );

        if (memcmp(list[i].bssid, targetBssid, 6) == 0) {
            found = true;
            channel = list[i].primary;
        }
    }

    free(list);

    if (!found) {
        Serial.println("Bevorzugte BSSID nicht im Scan gefunden");
        return false;
    }

    Serial.printf("Bevorzugte BSSID gefunden, Kanal %d\n", channel);

    // ESP-IDF STA Config direkt setzen
    wifi_config_t cfg = {};
    strncpy((char*)cfg.sta.ssid, ssid, sizeof(cfg.sta.ssid));
    strncpy((char*)cfg.sta.password, pass, sizeof(cfg.sta.password));

    memcpy(cfg.sta.bssid, targetBssid, 6);
    cfg.sta.bssid_set = true;

    // Kanal NICHT setzen → Auto-Scan aktiv
    cfg.sta.channel = 0;

    esp_wifi_disconnect();
    esp_wifi_set_config(WIFI_IF_STA, &cfg);
    esp_wifi_connect();

    // Sleep deaktivieren
    esp_wifi_set_ps(WIFI_PS_NONE);
    WiFi.setSleep(false);

    unsigned long start = millis();
    while (millis() - start < 7000) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Verbunden mit bevorzugter BSSID!");
            return true;
        }
        delay(100);
    }

    Serial.println("Bevorzugte BSSID nicht erreichbar");
    return false;
}

bool tryNormalConnect(unsigned long timeout) {

    // SSID + Passwort aus ESP-IDF holen
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);

    const char* ssid = (const char*)conf.sta.ssid;
    const char* pass = (const char*)conf.sta.password;

    Serial.print("Versuche normale Verbindung mit SSID: ");
    Serial.println(ssid);

    if (strlen(ssid) == 0) {
        Serial.println("SSID aus NVS ist leer → keine normale Verbindung möglich");
        return false;
    }

    WiFi.disconnect(false);
    delay(200);

    WiFi.begin(ssid, pass);
    WiFi.setSleep(false);

    unsigned long start = millis();
    while (millis() - start < timeout) {
        if (WiFi.status() == WL_CONNECTED) return true;
        delay(100);
    }

    Serial.println("Normale Verbindung fehlgeschlagen");
    return false;
}


void WifiManager::begin(char const *apName, unsigned long newTimeout)
{
    captivePortalName = apName;
    timeout = newTimeout;
    _newwificallback = NULL;
    NVSManager.begin();
    serverRunning = true;

    WiFi.onEvent(WiFiEvent);
    WiFi.mode(WIFI_STA);
    WiFi.persistent(true);
    WiFi.setAutoReconnect(true);

    // Static IP?
    ip = IPAddress(configManager.internal.ip);
    gw = IPAddress(configManager.internal.gw);
    sub = IPAddress(configManager.internal.sub);
    dns = IPAddress(configManager.internal.dns);

    if (isIPAddressSet(ip) || isIPAddressSet(gw) || isIPAddressSet(sub) || isIPAddressSet(dns)) {
        Serial.println("Using static IP");
        WiFi.config(ip, gw, sub, dns);
    }

    // SSID aus NVS holen
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);
    String configSsid = F(conf.sta.ssid);
    String configPsk  = F(conf.sta.password);
    NVSManager.GetString("PREFBSSID",&bssid,"");


    Serial.println("configSsid=");
    Serial.println(configSsid);

    if (configSsid == "") {
        Serial.println("Configured SSID is empty.");
        // Timeout runter setzen
        timeout = 2000;
        startCaptivePortal(captivePortalName);
        return;
    }

    // 1) Versuch: bevorzugte BSSID
    if (bssid != "" && bssid.length() >= 17) {
        if (tryPreferredBssid(bssid)) {
            Serial.print("Connected via preferred BSSID, IP: ");
            Serial.println(WiFi.localIP());
            return;
        }
        // WICHTIG: WiFi-Stack sauber zurücksetzen
        Serial.println("Reset WiFi-Stack nach BSSID-Timeout...");
        esp_wifi_disconnect();
        esp_wifi_stop();
        delay(100);
        esp_wifi_start();
        delay(100);
        WiFi.mode(WIFI_STA);
        WiFi.setSleep(false);
        WiFi.setAutoReconnect(true);
    }

    // 2) Versuch: normale Verbindung
    if (tryNormalConnect(timeout)) {
        Serial.print("Connected via SSID, IP: ");
        Serial.println(WiFi.localIP());
        return;
    }

    // 3) Fallback: Captive Portal
    Serial.print(PSTR("Timed out waiting for connect. Starting captive portal. SSID: "));
    Serial.println(WiFi.SSID());
    startCaptivePortal(captivePortalName);
}

bool WifiManager::forceReconnectIfIpLost()
{
    // 1) Prüfen ob IP verloren oder nicht verbunden
    if (WiFi.status() == WL_CONNECTED && WiFi.localIP().toString() != "0.0.0.0") {
        // Alles ok → kein Reconnect nötig
        DiagManager.PushDiagData(msgFehler,"Fehler: Reconnect aufgerufen obwohl Connectiviät besteht");
        return true;
    }

    Serial.println("forceReconnectIfIpLost: IP verloren oder nicht verbunden → Reconnect starten");

    // 2) WiFi-Stack sauber resetten
    esp_wifi_disconnect();
    esp_wifi_stop();
    delay(100);
    esp_wifi_start();
    delay(100);

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);

    // 3) SSID + Passwort aus NVS holen
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);

    String ssid = String((char*)conf.sta.ssid);
    String pass = String((char*)conf.sta.password);

    if (ssid.length() == 0) {
        Serial.println("forceReconnectIfIpLost: Keine SSID gespeichert → Captive Portal");
        startCaptivePortal(captivePortalName);
        return false;
    }

    // 4) bevorzugte BSSID versuchen
    if (bssid != "" && bssid.length() >= 17) {
        Serial.println("forceReconnectIfIpLost: Versuche bevorzugte BSSID…");

        if (tryPreferredBssid(bssid)) {
            Serial.println("forceReconnectIfIpLost: Erfolgreich über bevorzugte BSSID verbunden!");
            Serial.println(WiFi.localIP());
            return true;
        }

        Serial.println("forceReconnectIfIpLost: BSSID-Timeout → Stack Reset");
        esp_wifi_disconnect();
        esp_wifi_stop();
        delay(100);
        esp_wifi_start();
        delay(100);
        WiFi.mode(WIFI_STA);
        WiFi.setSleep(false);
        WiFi.setAutoReconnect(true);
    }

    // 5) normale Verbindung
    Serial.println("forceReconnectIfIpLost: Versuche normale SSID-Verbindung…");
    if (tryNormalConnect(7000)) {
        Serial.println("forceReconnectIfIpLost: Erfolgreich über SSID verbunden!");
        Serial.println(WiFi.localIP());
        return true;
    }

    // 6) Fallback: Captive Portal
    Serial.println("forceReconnectIfIpLost: Verbindung englueltig fehlgeschlagen");
    // startCaptivePortal(captivePortalName);
    return false;
}


void WifiManager::connectNewWifi(String newSSID, String newPass, String newBssid)
{
    Serial.println("ConnectNewWifi: Neue Daten empfangen");
    Serial.printf("SSID: %s | PASS: %s | BSSID: %s\n",
                  newSSID.c_str(), newPass.c_str(), newBssid.c_str());

    // Static IP setzen
    ip = IPAddress(configManager.internal.ip);
    gw = IPAddress(configManager.internal.gw);
    sub = IPAddress(configManager.internal.sub);
    dns = IPAddress(configManager.internal.dns);

    if (isIPAddressSet(ip) || isIPAddressSet(gw) || isIPAddressSet(sub) || isIPAddressSet(dns)) {
        Serial.println("Using static IP");
        WiFi.config(ip, gw, sub, dns);
    }

    // Verbindung abbrechen
    esp_wifi_disconnect();
    delay(100);

    // Neue Credentials in ESP-IDF STA Config schreiben
    wifi_config_t cfg = {};
    strncpy((char*)cfg.sta.ssid, newSSID.c_str(), sizeof(cfg.sta.ssid));
    strncpy((char*)cfg.sta.password, newPass.c_str(), sizeof(cfg.sta.password));

    // BSSID optional setzen
    if (newBssid != "" && newBssid.length() >= 17) {
        uint8_t targetBssid[6];
        if (parseMac(newBssid.c_str(), targetBssid)) {
            memcpy(cfg.sta.bssid, targetBssid, 6);
            cfg.sta.bssid_set = true;
            cfg.sta.channel = 0;  // Auto-Scan
            Serial.println("ConnectNewWifi: BSSID gesetzt");
        } else {
            Serial.println("ConnectNewWifi: BSSID ungültig → ignoriert");
            cfg.sta.bssid_set = false;
        }
    } else {
        cfg.sta.bssid_set = false;
    }

    // Config setzen
    esp_wifi_set_config(WIFI_IF_STA, &cfg);

    // 1) Versuch: bevorzugte BSSID
    if (cfg.sta.bssid_set) {
        Serial.println("ConnectNewWifi: Versuche bevorzugte BSSID…");

        if (tryPreferredBssid(newBssid)) {
            Serial.println("ConnectNewWifi: Erfolgreich über bevorzugte BSSID verbunden!");
            Serial.println(WiFi.localIP());
            storeToEEPROM();
            if (_newwificallback) 
                _newwificallback();
            if (inCaptivePortal)
                stopCaptivePortal();
            return;
        }

        // Reset wie in begin()
        Serial.println("ConnectNewWifi: BSSID-Timeout → Reset WiFi-Stack…");
        esp_wifi_disconnect();
        esp_wifi_stop();
        delay(100);
        esp_wifi_start();
        delay(100);
        WiFi.mode(WIFI_STA);
        WiFi.setSleep(false);
        WiFi.setAutoReconnect(true);
    }

    // 2) Versuch: normale Verbindung
    Serial.println("ConnectNewWifi: Versuche normale SSID-Verbindung…");
    WiFi.begin(newSSID.c_str(), newPass.c_str());
    unsigned long start = millis();
    while (millis() - start < 10000) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("ConnectNewWifi: Erfolgreich über SSID verbunden!");
            Serial.println(WiFi.localIP());
            storeToEEPROM();
            if (_newwificallback) 
                _newwificallback();
            if (inCaptivePortal)
                stopCaptivePortal();
            return;
        }
        delay(100);
    }

    Serial.println("ConnectNewWifi: SSID-Verbindung fehlgeschlagen");

    // 3) Fallback: Captive Portal
    Serial.println("ConnectNewWifi: Starte Captive Portal…");
    startCaptivePortal(captivePortalName);
}


//function to forget current WiFi details and start a captive portal
void WifiManager::forget()
{ 
    Serial.println("forgetWifi…");

    WiFi.persistent(true);
    WiFi.disconnect(false,true);   // löscht SSID/Passwort
    NVSManager.WriteString("PREFBSSID", String("")); // optional: BSSID löschen
    // optional: explizit auch ESP-IDF config nullen
    wifi_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    esp_wifi_set_config(WIFI_IF_STA, &cfg);
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

    delay(200);
    // optional reboot:
    Serial.println(PSTR("Restart wird durchgeführt"));
    ESP.restart();
}

//function to request a connection to new WiFi credentials
void WifiManager::setNewWifi(String newSSID, String newPass, String newBssid)
{    
    ssid = newSSID;
    pass = newPass;
    bssid = newBssid;
    ip = IPAddress();
    sub = IPAddress();
    gw = IPAddress();
    dns = IPAddress();
    NVSManager.WriteString("PREFBSSID",newBssid);

    Serial.println(PSTR("New BSSID: ") + newBssid + PSTR(",  Will reconnect."));
    reconnect = true;
}

//function to request a connection to new WiFi credentials
void WifiManager::setNewWifi(String newSSID, String newPass, String newBssid, String newIp, String newSub, String newGw, String newDns)
{
    ssid = newSSID;
    pass = newPass;
    bssid = newBssid;
    ip.fromString(newIp);
    sub.fromString(newSub);
    gw.fromString(newGw);
    dns.fromString(newDns);
    NVSManager.WriteString("PREFBSSID",newBssid);
    reconnect = true;
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

String WifiManager::PASS()
{    
    return WiFi.psk();
}


long WifiManager::RSSI()
{    
    return WiFi.RSSI();
}

String WifiManager::BSSID()
{    
    return WiFi.BSSIDstr();
}

String WifiManager::ConfBSSID()
{    
    return bssid;
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
        connectNewWifi(ssid, pass,bssid);
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

