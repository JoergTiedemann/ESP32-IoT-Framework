#include "webServer.h"
#include "FirebaseJson.h"
#include <Arduino.h>

#include "FS.h"
#include <LittleFS.h>
#include "esp_core_dump.h"



// Include the header file we create with webpack
#include "../../../../../src/generated/html.h"

//Access to other classes for GUI functions
#include "WiFiManager.h"
#include "configManager.h"
#include "updater.h"
#include "dashboard.h"
#include "DiagManager.h"
#include "OTAManager.h"
#include "BoardsInformation.h"


const esp_partition_t* getCoredumpPartition() {
    esp_partition_iterator_t it = esp_partition_find(
        ESP_PARTITION_TYPE_DATA,
        ESP_PARTITION_SUBTYPE_DATA_COREDUMP,
        NULL
    );
    if (!it) return nullptr;

    const esp_partition_t* p = esp_partition_get(it);
    esp_partition_iterator_release(it);
    return p;
}


static String AnalyzeCoreDump() {

    String out;
    // Reset-Grund auslesen (für Watchdogs wichtig!)
    esp_reset_reason_t reason = esp_reset_reason();
    out += "Reset Reason: ";
    switch (reason) {
        case ESP_RST_INT_WDT:   out += "Interrupt Watchdog\n"; break;
        case ESP_RST_TASK_WDT:  out += "Task Watchdog\n"; break;
        case ESP_RST_WDT:       out += "General Watchdog\n"; break;
        case ESP_RST_BROWNOUT:  out += "Brownout Reset\n"; break;
        case ESP_RST_PANIC:     out += "CPU Exception (Panic)\n"; break;
        case ESP_RST_SDIO:      out += "Reset over SDIO\n"; break;
        case ESP_RST_DEEPSLEEP: out += "Reset after exiting deep sleep mode\n"; break;
        case ESP_RST_SW:        out += "Software reset via esp_restart\n"; break;
        case ESP_RST_EXT:       out += "Reset by external pin\n"; break;
        case ESP_RST_POWERON:   out += "Reset due to power-on event\n"; break;
        case ESP_RST_UNKNOWN:   out += "Reset reason can not be determined\n"; break;
        default:                out += "Other / Unknown\n"; break;
    }

    // Initialisiere das Coredump-Subsystem im Flash
    esp_core_dump_init();

    // 1. Prüfen, ob überhaupt ein gültiger Coredump vorliegt
    if (esp_core_dump_image_check() == ESP_OK) {
        out += String("Gueltiger Coredump im Flash gefunden.");

        // Struktur für die Zusammenfassung allokieren
        esp_core_dump_summary_t summary;

        // 2. Zusammenfassung aus dem Coredump parsen
        if (esp_core_dump_get_summary(&summary) == ESP_OK) {
            
            // Da der ESP32-S3 ein Xtensa-Chip ist, greifen wir auf .ex_info.exc_cause zu
            uint32_t exccause = summary.ex_info.exc_cause;
            
            // Serial.printf(">>> ABSTURZURSACHE (EXCCAUSE): 0x%02X (%d) <<<\n", exccause, exccause);

            // Optionale Zusatzinfos, die ebenfalls in der Summary stecken:
            char line[256];
            snprintf(line, sizeof(line),
                "Abgestuerzter Task: %s\nProgram Counter (PC): 0x%08X\n",
                summary.exc_task,
                (unsigned int)summary.exc_pc
            );
            out += String(line);
            // Serial.printf("Abgestuerzter Task: %s\n", summary.exc_task);
            // Serial.printf("Program Counter (PC): 0x%08X\n", (unsigned int)summary.exc_pc);
            // Serial.printf("Gefährliche Adresse (EXCVADDR): 0x%08X\n", (unsigned int)summary.ex_info.exc_vaddr);

            // Exception-Typ interpretieren
            if (reason == ESP_RST_INT_WDT) {
                out += "Watchdog: Interrupt-WDT hat ausgeloest.\n";
                return out;
            }
            if (reason == ESP_RST_TASK_WDT) {
                out += "Watchdog: Task-WDT hat ausgeloest.\n";
                return out;
            }
            if (reason == ESP_RST_WDT) {
                out += "Watchdog: General-WDT hat ausgeloest.\n";
                return out;
            }

            // Bekannte Xtensa-Fehlercodes übersetzen
            switch (exccause) {
                case 0:
                    out += String("Bedeutung: IllegalInstructionCause (ungueltiger Befehl / z.B. kaputter Funktionszeiger)");
                    break;
                case 1:  
                    out += String("Hardware-Exception: Syscall (Systemaufruf)");
                    break;
                case 2:  
                    out += String("Hardware-Exception: InstructionFetchError (Fehler beim Laden des Befehls)");
                    break;
                case 3:
                    out += String("Bedeutung: LoadStoreErrorCause (Fehler beim Speicherzugriff / Bus-Fehler)");
                    break;
                case 4:  
                    out += String("Hardware-Exception: Level1Interrupt (Gewoehnlicher CPU-Interrupt)");
                    break;
                case 5:  
                    out += String("Hardware-Exception: AllocaError (Interner Stack-Fenster-Fehler)");
                    break;
                case 6:
                    out += String("Bedeutung: DivideByZeroCause (Division durch Null)");
                    break;
                case 8:  
                    out += String("Hardware-Exception: PrivilegedInstruction (Rechte-Verletzung)");
                    break;
                case 9:
                    out += String("Bedeutung: LoadStoreAlignmentCause (nicht ausgerichteter Speicherzugriff)");
                    break;
                case 12: out += String("Hardware-Exception: InstrDataError (Hardware-Uebertragungsfehler Befehl)");
                    break;
                case 13: out += String("Hardware-Exception: LoadDataError (Hardware-Uebertragungsfehler Lesen)");
                    break;
                case 14: out += String("Hardware-Exception: StoreDataError (Hardware-Uebertragungsfehler Schreiben)");
                    break;
                case 15: out += String("Hardware-Exception: InstrAddrError (Ungueltige Befehlsadresse / Flash-Sprung)");
                    break;
                case 16: out += String("Hardware-Exception: LoadAddrError (Ungueltige Leseadresse)");
                    break;
                case 17: out += String("Hardware-Exception: StoreAddrError (Ungueltige Schreibadresse)");
                    break;
                case 18: out += String("Hardware-Exception: ITLBMiss (Virtuelle Befehlsadresse nicht in MMU gefunden)");
                    break;
                case 19: out += String("Hardware-Exception: ITLBMultiHit (Schwerer MMU-Konfigurationsfehler)");
                    break;
                case 20: out += String("Hardware-Exception: ITLBAccessViolation (Zugriff verweigert / No-Execute Speicher)");
                    break;
                case 22: out += String("Hardware-Exception: DTLBMiss (Virtuelle Datenadresse nicht in MMU gefunden)");
                    break;
                case 23: out += String("Hardware-Exception: DTLBMultiHit (Schwerer MMU-Konfigurationsfehler Daten)");
                    break;
                case 24: out += String("Hardware-Exception: DTLBAccessViolation (Keine Leserechte fuer diesen Speicherbereich)");
                    break;
                case 25: out += String("Hardware-Exception: DTLBMuStoreProhibited (Schreibschutz-Verletzung)");
                    break;
                case 28:
                    out += String("Bedeutung: LoadProhibitedCause (Lesen von geschuetzter/Null-Adresse)");
                    break;
                case 29:
                    out += String("Bedeutung: StoreProhibitedCause (Schreiben auf geschuetzte/Null-Adresse)");
                    break;
                case 32: 
                case 33: 
                case 34: 
                case 35:
                case 36: 
                case 37: 
                case 38: 
                case 39: 
                     out += String("Hardware-Exception: CoprocessorDisabled (FPU oder KI-Beschleuniger deaktiviert)");
                    break;
                default: 
                    char buf[64];
                    snprintf(buf, sizeof(buf),
                            "Bedeutung: Andere/Unbekannte CPU-Exception (Code %u)\n",
                            exccause);
                    out += buf;
                    break;
                }
        } else {
            out += String("Fehler beim Lesen der Coredump-Zusammenfassung.");
        }

        // 3. WICHTIG: Coredump löschen, sonst liest du ihn bei jedem Boot erneut aus!
        // esp_core_dump_image_erase();
        
    } else {
        out += String("Kein Coredump vorhanden.");
    }
  return out;
}



void webServer::begin(UpdateDiagDisplayCallback diagDisplayCallback,GetDiagDataCallback pGetDiagDataCallback)
{
#ifdef ESP32
    // TODO: Remove enter/exit traces after ESP32 build stable.  Experienced frequent crashing
    // during initial port when TCP stack not initialized before starting webserver instance.
    Serial.println(PSTR("webServer::begin enter"));
#endif
    m_GetDiagDataCallback = pGetDiagDataCallback;
    m_getLibraryVersionCallback = BoardInformation.SendLibraryVersion;
    m_diagDisplayCallback = diagDisplayCallback;
    m_bRestartActive = false;
    //to enable testing and debugging of the interface
    DefaultHeaders::Instance().addHeader(PSTR("Access-Control-Allow-Origin"), PSTR("*"));

    server.serveStatic("/download", LittleFS, "/");

    server.onNotFound(requestHandler);

    //handle uploads
    server.on(PSTR("/upload"), HTTP_POST, [](AsyncWebServerRequest *request) {}, handleFileUpload);

    bindAll();

#ifdef ESP32
    Serial.println(PSTR("Calling server.begin();  Will fail on ESP32 if tcp stack not initialized.  Ensure WiFiManager was called first."));
#endif

    server.addHandler(&ws);
    server.begin();

#ifdef ESP32
   Serial.println(PSTR("webServer::begin done."));
#endif
}

void webServer::WifiGetResult(String& JSON,const bool bScan)
{
    FirebaseJson jsonBuffer;
    // Serial.printf("Get Wifidata ActiveScan:%d Scan:%d\n",WiFiManager.m_bNetworkScan,bScan);

    jsonBuffer.set("captivePortal",WiFiManager.isCaptivePortal());
    jsonBuffer.set("ssid",WiFiManager.SSID());
    jsonBuffer.set("pass",WiFiManager.PASS());
    jsonBuffer.set("strength",String(WiFiManager.RSSI()));
    jsonBuffer.set("bssid",String(WiFiManager.BSSID()));
    jsonBuffer.set("confbssid",String(WiFiManager.ConfBSSID()));
    // Serial.printf("Get Wifi\n");
    String str,strenc;
    //nun noch die Wlanscandaten 
    FirebaseJsonArray wifiInfo;
//    Serial.printf("WifiGetResult ScanRunning:%d Done:%d\n", WiFiManager.m_bNetworkScanRunning, WiFiManager.m_bNetworkScanDone);
    if (WiFiManager.m_bNetworkScanRunning==true)
    {
            wifiInfo.add(String("Netzwerkscan läuft noch"));
    }
    else
    {
        if (WiFiManager.m_bNetworkScanDone==true)
        {
            WiFiManager.GetScanString(&wifiInfo);
        }
        if (bScan)
        {
            str = WiFiManager.StartNetworkscan();
            wifiInfo.add(str);
        }
    }
    jsonBuffer.add("wifiInfo",wifiInfo);
    JSON = jsonBuffer.raw();
}

bool diagnoseLogExists() {
    return LittleFS.exists("/diagnose.log");
}

void webServer::bindAll()
{
    //Restart the ESP
    server.on(PSTR("/api/restart"), HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, PSTR("text/html"), ""); //respond first because of restart
        Serial.printf("Restart via Webinterface\n");
        GUI.m_bRestartActive = true;
        ESP.restart();
    });

    //Switch Displaymode
    server.on(PSTR("/api/diagdisplay"), HTTP_POST, [](AsyncWebServerRequest *request) {
        if (GUI.m_diagDisplayCallback != NULL)
            GUI.m_diagDisplayCallback();
        request->send(200, PSTR("text/html"), ""); //respond first because of restart
    });

    //update WiFi details
    server.on(PSTR("/api/wifi/set"), HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, PSTR("text/html"), ""); //respond first because of wifi change
        WiFiManager.setNewWifi(request->arg("ssid"), request->arg("pass"), request->arg("bssid"));
    });

    //update WiFi details with static IP
    server.on(PSTR("/api/wifi/setStatic"), HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, PSTR("text/html"), ""); //respond first because of wifi change
        WiFiManager.setNewWifi(request->arg("ssid"), request->arg("pass"), request->arg("bssid"), request->arg("ip"), request->arg("sub"), request->arg("gw"), request->arg("dns"));
    });

    //update WiFi details
    server.on(PSTR("/api/wifi/forget"), HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, PSTR("text/html"), ""); //respond first because of wifi change
        WiFiManager.forget();
    });

    //get WiFi details
    server.on(PSTR("/api/wifi/get"), HTTP_GET, [](AsyncWebServerRequest *request) {
        String JSON;
        GUI.WifiGetResult(JSON,false);
        Serial.printf("Wifi Get response:%s<-Ende\n",JSON.c_str());
        // Serial.printf("ScanRunning:%d Done:%d\n", WiFiManager.m_bNetworkScanRunning, WiFiManager.m_bNetworkScanDone);
        request->send(200, PSTR("text/html"), JSON);
    });

    server.on(PSTR("/api/wifi/getScan"), HTTP_GET, [](AsyncWebServerRequest *request) {
        String JSON;
        GUI.WifiGetResult(JSON,true);
        Serial.printf("Wifi Getscan response:%s<-Ende\n",JSON.c_str());
        // Serial.printf("ScanRunning:%d Done:%d\n", WiFiManager.m_bNetworkScanRunning, WiFiManager.m_bNetworkScanDone);
        request->send(200, PSTR("text/html"), JSON);
    });

     //get diagnosedaten
    server.on(PSTR("/api/diagnosticdata/get"), HTTP_GET, [](AsyncWebServerRequest *request) {
        String JSON;
        FirebaseJson jsonBuffer;
        Serial.printf("Get Diagdata\n");

        FirebaseJson jsonDynamicDiagConfig;
        FirebaseJsonArray files;
        FirebaseJsonArray variablenames;
        FirebaseJsonArray variablevalues;

        //get diagnosticdata listing
        for (int i = 0;i < DiagManager.GetLogSize();i++)
        {
            String str;
            str = DiagManager.GetLogString(i);
            files.add(str);
        }

        //Feste Werte ins JSON buegeln
        // jsonBuffer["variabletxt1"] = String("ABC Bimbam");
        // jsonBuffer["variablevalue1"] = String("ABC");
        // jsonBuffer["variabletxt2"] = String("Testvariable");
        // jsonBuffer["variablevalue2"] = String("DEF");
        // jsonBuffer["variabletxt3"] = String("Variable 3");
        // jsonBuffer["variablevalue3"] = String("123");

        //Hier gehts weiter die Dataen muessen besetzt werden 
        // for (int i = 0;i < 4;i++)
        // {
        //     String str;
        //     str = "Variable" + String(i);
        //     variablenames.add(str);
        //     str = "Wert" + String((i+1)*10);
        //     variablevalues.add(str);
        // }

        for (int i = 0;i < DiagManager.GetVariableMonitorSize();i++)
        {
            variablenames.add(DiagManager.GetVariableMonitorName(i));
            variablevalues.add(DiagManager.GetVariableMonitorValue(i));
        }

        if (GUI.m_diagDisplayCallback)
           jsonDynamicDiagConfig.set("diagdisplay",String(true));
        else
           jsonDynamicDiagConfig.set("diagdisplay",String(false));

        if (diagnoseLogExists()) 
           jsonDynamicDiagConfig.set("diaglogfile",String(true));
        else
           jsonDynamicDiagConfig.set("diaglogfile",String(false));

        jsonBuffer.add("dynamicConfigdata",jsonDynamicDiagConfig);
        jsonBuffer.add("files",files);
        jsonBuffer.add("variablenames",variablenames);
        jsonBuffer.add("variablevalues",variablevalues);
        JSON = jsonBuffer.raw();
        if (GUI.m_GetDiagDataCallback)
        GUI.m_GetDiagDataCallback();

        // Serial.printf("Diagdata:%s<-Ende\n",JSON.c_str());
        request->send(200, PSTR("text/html"), JSON);

    });

    //get firmwareurl
    server.on(PSTR("/api/firmwareurl/get"), HTTP_GET, [](AsyncWebServerRequest *request) {
        GUI.m_LibraryVersionInfo.request = request;
        GUI.m_getLibraryVersionCallback(GUI.m_LibraryVersionInfo);
        // String JSON;
        // StaticJsonDocument<1000> jsonBuffer;
        // jsonBuffer["firmwareurl"] = String(configManager.data.FirmwareURL); 
        // serializeJson(jsonBuffer, JSON);
        // Serial.printf("api/firmwareurl/get:%s<-Ende\n",JSON.c_str());
        // request->send(200, PSTR("text/html"), JSON);
    });
    //do Firmwareupdate via firmwareurl 
    server.on(PSTR("/api/firmwareurl/DoUpdate"), HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, PSTR("text/html"), ""); //respond first because of restart
        Serial.printf("Anforderung von FirmwareUpdate\n");
        OTAManager.m_bDoUpdate = true;
    });


    //get file listing
    server.on(PSTR("/api/files/get"), HTTP_GET, [](AsyncWebServerRequest *request) {

        String JSON;
        FirebaseJson jsonBuffer;
        FirebaseJsonArray files;

#ifdef ESP32
        //get file listing

        // 'Dir' and 'LittleFS.openDir' not implemented in ESP32, instead dir and 
        // files are represented as 'File'.  See docs/example:
        // https://github.com/lorol/ESPAsyncWebServer/blob/8c77d0e63f55160953fda843baa11435b05ae0bd/src/SPIFFSEditor.cpp#L187
        File dir = LittleFS.open("/");
        File entry = dir.openNextFile();
        
        // Copy enumerated filenames to temp array until serialized.  Reason... String mem refs 
        // returned by entry.name() could be reused, resulting in garbage chars returned to clients.
        StringArray strFiles;

        while (entry)
        {
            String fileName = String(entry.name());
            strFiles.add(fileName);
            files.add((strFiles.nth(strFiles.length() - 1))->c_str());
            entry = dir.openNextFile();
        }
#elif defined(ESP8266)
        //get file listing
        Dir dir = LittleFS.openDir("");
        while (dir.next())
            files.add(dir.fileName());
#endif

#ifdef ESP32
        //get used and total data

        // NOTE: esp_littlefs_info called twice, paying file block traversal each time.  Meh, that's
        // OK for small # of files.
        jsonBuffer.set("used",String(LittleFS.usedBytes())); 
        jsonBuffer.set("max",String(LittleFS.totalBytes()));
#elif defined(ESP8266)
        //get used and total data
        FSInfo fs_info;
        LittleFS.info(fs_info);
        jsonBuffer["used"] = String(fs_info.usedBytes);
        jsonBuffer["max"] = String(fs_info.totalBytes);
#endif
        jsonBuffer.add("files",files);
        JSON = jsonBuffer.raw();
#ifdef ESP32
        strFiles.free();
#endif

        request->send(200, PSTR("text/html"), JSON);
    });

    //remove file
    server.on(PSTR("/api/files/remove"), HTTP_POST, [](AsyncWebServerRequest *request) {
        LittleFS.remove("/" + request->arg("filename"));
        request->send(200, PSTR("text/html"), "");
    });

    //update from LittleFS
    server.on(PSTR("/api/update"), HTTP_POST, [](AsyncWebServerRequest *request) {        
        updater.requestStart("/" + request->arg("filename"));
        request->send(200, PSTR("text/html"), "");
    });

    //update status
    server.on(PSTR("/api/update-status"), HTTP_GET, [](AsyncWebServerRequest *request) {
        String JSON;
        FirebaseJson jsonBuffer;

        jsonBuffer.set("status",updater.getStatus());
        JSON = jsonBuffer.raw();
        request->send(200, PSTR("text/html"), JSON);
    });

    //send binary configuration data
    server.on(PSTR("/api/config/get"), HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream(PSTR("application/octet-stream"));
        response->write(reinterpret_cast<char*>(&configManager.data), sizeof(configManager.data));
        request->send(response);
    });

    //receive binary configuration data from body
    server.on(
        PSTR("/api/config/set"), HTTP_POST,
        [this](AsyncWebServerRequest *request) {},
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {},
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            
            static uint8_t buffer[sizeof(configManager.data)];
            static uint32_t bufferIndex = 0;

            for (size_t i = 0; i < len; i++)
            {
                buffer[bufferIndex] = data[i];
                bufferIndex++;
            }

            if (index + len == total)
            {
                bufferIndex = 0;
                configManager.saveRaw(buffer);
                request->send(200, PSTR("text/html"), "");
            }

        });

    //receive binary configuration data from body
    server.on(
        PSTR("/api/dash/set"), HTTP_POST,
        [this](AsyncWebServerRequest *request) {},
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {},
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            // Serial.printf("Set Dashdata start:%d Laenge:%d\n",(request->arg("start")).toInt(),(request->arg("length")).toInt());
            memcpy(reinterpret_cast<uint8_t *>(&(dash.data)) + (request->arg("start")).toInt(), data, (request->arg("length")).toInt());
            request->send(200, PSTR("text/html"), "");
            // Serial.printf("inputwitchon:%d \n",dash.data.TestDoubleButtonON);
            // Serial.printf("inputbuttoff:%d \n",dash.data.TestDoubleButtonOfff);
            // dash.data.TestDoubleButtonON = 0;
            // dash.data.TestDoubleButtonOfff = 0;
        });

        // Webserver-Endpoint for coredump information, deletion, and binary download
        server.on("/coredump.info", HTTP_GET, [](AsyncWebServerRequest *request){
            const esp_partition_t* p = getCoredumpPartition();
            if (!p) {
                request->send(404, "text/plain", "No coredump partition found");
                return;
            }
            String info = AnalyzeCoreDump();
            request->send(200, "text/plain", info);
        });
        server.on("/coredump.delete", HTTP_GET, [](AsyncWebServerRequest *request){
            const esp_partition_t* p = getCoredumpPartition();
            if (!p) {
                request->send(404, "text/plain", "No coredump partition found");
                return;
            }
            esp_partition_erase_range(p, 0, p->size);
            request->send(200, "text/plain", "Coredump deleted");
        });

        server.on("/coredump.bin", HTTP_GET, [](AsyncWebServerRequest *request){
            const esp_partition_t* p = getCoredumpPartition();
            if (!p) {
                request->send(404, "text/plain", "No coredump partition found");
                return;
            }

            uint32_t marker;
            esp_partition_read(p, 0, &marker, sizeof(marker));

            if (marker == 0xFFFFFFFF) {
                request->send(404, "text/plain", "No coredump available");
                return;
            }

            // Dump existiert → streamen
            AsyncResponseStream *response =
                request->beginResponseStream("application/octet-stream");
            response->addHeader("Content-Disposition",
                                "attachment; filename=\"coredump.bin\"");

            const size_t chunk = 4096;
            uint8_t buffer[chunk];

            size_t remaining = p->size;
            size_t offset = 0;

            while (remaining > 0) {
                size_t toRead = remaining > chunk ? chunk : remaining;

                if (esp_partition_read(p, offset, buffer, toRead) != ESP_OK) {
                    response->print("READ ERROR");
                    break;
                }

                response->write(buffer, toRead);

                offset += toRead;
                remaining -= toRead;
            }

            request->send(response);
        });
}

// Callback for the html
void webServer::serveProgmem(AsyncWebServerRequest *request)
{    
    // Dump the byte array in PROGMEM with a 200 HTTP code (OK)
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", html, html_len);

    // Tell the browswer the content is Gzipped
    response->addHeader(PSTR("Content-Encoding"), PSTR("gzip"));
    
    request->send(response);    
}

void webServer::handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    static File fsUploadFile;

    if (!index)
    {
        if (!filename.startsWith("/"))
            filename = "/" + filename;

        Serial.print(PSTR("Start file upload, filename="));
        Serial.println(filename);

        fsUploadFile = LittleFS.open(filename, "w");

#ifdef ESP32
        // Open write fail?
        if (!LittleFS.exists(filename))
        {
            Serial.println(PSTR("[E] LittleFS.open() failed.  Is LittleFS initialized?  Was updater.begin() called?"));
        }
#endif

    }

    for (size_t i = 0; i < len; i++)
    {
        fsUploadFile.write(data[i]);
    }

    if (final)
    {
        String JSON;
        FirebaseJson jsonBuffer;
#ifdef ESP32
        if (!filename.startsWith("/"))
            filename = "/" + filename;

        jsonBuffer.set("success",LittleFS.exists(filename));
#elif defined(ESP8266)
        jsonBuffer.set("success",fsUploadFile.isFile());
#endif
        JSON = jsonBuffer.raw();
        request->send(200, PSTR("text/html"), JSON);
        fsUploadFile.close();
    }
}

webServer GUI;