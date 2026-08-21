#include <Arduino.h>
#include "esp_timer.h"
#include "FS.h"
#include <LittleFS.h>


#include "WiFiManager.h"
#include "webServer.h"
#include "updater.h"
#include "configManager.h"
#include "timeSync.h"

#include "dashboard.h"
#include "DiagManager.h"
#include "BoardsInformation.h"
#include "OTAManager.h"

String strTime("No Time");
String strBootTime("");
long m_lLoopCounter = 0;
long m_FreeHeap = 0;

const long  clNTPServerCount = 4; 
const char* ntpServers[] = {
    "0.pool.ntp.org",
    "1.pool.ntp.org",
    "2.pool.ntp.org",
    "time.google.com"
};



struct task
{    
    unsigned long rate;
    unsigned long previous;
};

task taskA = { .rate = 500, .previous = 0 };

void setup() 
{
     Serial.begin(115200);

    BoardInformation.PrintBoardInformation();
    BoardInformation.print_used_libraries();
    LittleFS.begin();
    updater.begin();
    configManager.begin();
    // configManager.setConfigSaveCallback(&xMutexFirebase,saveCallback);
    WiFiManager.begin(configManager.data.projectName);

    GUI.begin();
    // GUI.begin();

    DiagManager.begin(20,18);

    Serial.println("Hello world, setup");

    // Zeitzone Berlin
    NTPSync::instance().begin(
        "CET-1CEST,M3.5.0/02:00:00,M10.5.0/03:00:00",
        ntpServers,
        clNTPServerCount,
        10   // Retry alle 10 Sekunden
    );
    strTime = NTPSync::instance().GetTime();
    Serial.print(PSTR("Current time in Berlin: "));
    Serial.println(strTime);
    strBootTime = strTime;


    dash.begin(500);
    OTAManager.begin();
    //ueberwachung immer einschalten nach Neustart -> nein wollen wir erstmal nicht
    DiagManager.AddVariableToMonitor(0,String("Letzter Systemstart"),&strBootTime);
    DiagManager.AddVariableToMonitor(1,String("Sytemzeit"),&strTime);
 
    DiagManager.AddVariableToMonitor(2,String("FB-Counter"),&m_lLoopCounter);
    DiagManager.AddVariableToMonitor(3,String("Free Heap"),&m_FreeHeap);

    // now = time(nullptr);
    // timeinfo = localtime(&now);  
}

void loop() 
{
    //software interrupts
    WiFiManager.loop();
    updater.loop();
    configManager.loop();
    dash.loop();
    strTime = NTPSync::instance().GetTime();

    //your code here
    //task A
    if (taskA.previous == 0 || (millis() - taskA.previous > taskA.rate))
    {
        taskA.previous = millis();
        m_lLoopCounter++;
        if (m_lLoopCounter % 10 == 0) // Log every 10 iterations
        {
            String strLog = "Loop Counter: "+String(m_lLoopCounter);
            DiagManager.PushDiagData(msgMeldung,strLog);
        }

        m_FreeHeap = ESP.getFreeHeap();

        String stringOne = "Apples";
        stringOne.toCharArray(dash.data.projectName,32);
        
        dash.data.dummyInt++;
        dash.data.inputInt++;

        dash.data.dummyFloat = sin((float)millis()/1000);

        if (dash.data.inputBool)
            dash.data.dummyBool = true;
        else
            dash.data.dummyBool = false;
    }
}
