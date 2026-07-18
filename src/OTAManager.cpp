 
#include "OTAManager.h"
#include <Update.h>


#include "configManager.h"
#include "DiagManager.h"

COTAManager OTAManager;

//function to call in setup
void COTAManager::begin()
{
      m_bDoUpdate = false;;

}
// Function to update firmware incrementally
// Buffer is declared to be 128 so chunks of 128 bytes
// from firmware is written to device until server closes
void COTAManager::updateFirmware(uint8_t *data, size_t len)
{
  Update.write(data, len);
  currentLength += len;
  // Print dots while waiting for update to finish
  Serial.print('.');
  // if current length of written firmware is not equal to total firmware size, repeat
  if(currentLength != totalLength) 
    return;
  Update.end(true);
  Serial.printf("\nUpdate Success, Total Size: %u\nRebooting...\n", currentLength);
  DiagManager.PushDiagData(msgAll,"Firmwareupdate erfolgreich");
  // Restart ESP32 to see changes 
  ESP.restart();
}


void COTAManager::DoOTAUpdate(String strHOST)
{
  // if ((configManager.data.UpdateFirmwareConnectToCloud) && (Firebase.ready()))
  currentLength = 0; //current size of written firmware
  m_bDoUpdate= false;
  // Connect to external web server
  Serial.print("Firmware update from: ");
  Serial.println(strHOST);
  String strFWUpdate = "FirmwareUpdate von "+strHOST+" gestartet";
  DiagManager.PushDiagData(msgAll,strFWUpdate);
  
  client.begin(strHOST);
  // Get file, just to check if each reachable
  int resp = client.GET();
  Serial.print("Response: ");
  Serial.println(resp);
  // If file is reachable, start downloading
  if(resp == 200){
      // get length of document (is -1 when Server sends no Content-Length header)
      totalLength = client.getSize();
      // transfer to local variable
      int len = totalLength;
      // this is required to start firmware update process
      Update.begin(UPDATE_SIZE_UNKNOWN);
      Serial.printf("FW Size: %u\n",totalLength);
      // create buffer for read
      uint8_t buff[128] = { 0 };
      // get tcp stream
      WiFiClient * stream = client.getStreamPtr();
      // read all data from server
      Serial.println("Updating firmware...");
      while(client.connected() && (len > 0 || len == -1)) {
           // get available data size
           size_t size = stream->available();
           if(size) {
              // read up to 128 byte
              int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
              // pass to function
              updateFirmware(buff, c);
              if(len > 0) {
                 len -= c;
              }
           }
           delay(1);
      }
  }else{
    String strFWError  = "Cannot download firmware file. Only HTTP response 200: OK is supported. Firmware URL ueberpruefen !";
    DiagManager.PushDiagData(msgFehler,strFWError);
    Serial.println("Cannot download firmware file. Only HTTP response 200: OK is supported. Double check firmware location #defined in HOST.");
  }
  client.end();
  
}