#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <ESPAsyncWebServer.h>
#include "generated/dash.h"
#include "historicdata.h"
const int cihistoricdatalength = 10;

class dashboard
{

private:
    static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    unsigned long loopRate = 0;
    unsigned long loopPrevious = 0;

public:
    void begin(int sampleTimeMs = 1000);
    void loop();
    void send();
    dashboardData data;
    dashboardHistoricData historicdata1[cihistoricdatalength];
    dashboardHistoricData historicdata2[cihistoricdatalength];

};

extern dashboard dash;

#endif