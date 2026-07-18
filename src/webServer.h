#ifndef SERVER_H
#define SERVER_H

#include <ESPAsyncWebServer.h>

typedef struct webserver_library_info_t
{
    AsyncWebServerRequest *request;
	String m_strPlatformVersion;
	String m_strFrameworkVersion;
	String m_strSDKVersion;
	String m_strLibraryVersion;
    
} WebserverLibraryInfo;
typedef void (*WebserverGetLibraryVersionCallback)(WebserverLibraryInfo);  
typedef void (*UpdateDiagDisplayCallback)();
typedef void (*GetDiagDataCallback)(); 

class webServer
{

private:    
    static void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    static void serveProgmem(AsyncWebServerRequest *request);
    void bindAll();
 public:
    GetDiagDataCallback m_GetDiagDataCallback = NULL;
    WebserverGetLibraryVersionCallback m_getLibraryVersionCallback = NULL;
    UpdateDiagDisplayCallback m_diagDisplayCallback = NULL;
    WebserverLibraryInfo m_LibraryVersionInfo;  
    bool m_bRestartActive = false;
    void WifiGetResult(String& JSON,const bool bScan);
    AsyncWebServer server;
    AsyncWebSocket ws;
    ArRequestHandlerFunction requestHandler = serveProgmem;
    void begin(WebserverGetLibraryVersionCallback getLibraryVersionCallback = NULL,UpdateDiagDisplayCallback diagDisplayCallback = NULL,GetDiagDataCallback pGetDiagDataCallback = NULL);
    webServer():server(80),ws("/ws"){};
};

extern webServer GUI;

#endif
