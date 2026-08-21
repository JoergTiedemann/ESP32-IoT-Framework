#include "BoardsInformation.h"

#include "spi_flash_mmap.h"
#include "esp_ota_ops.h"
#include "esp_image_format.h"
#include "configManager.h"
#include <FirebaseJson.h>
#include "..\..\..\..\..\src/generated\platformio_used_libraries.h"


/* use provided PLATFORMIO_USED_LIBRARIES macro */ 
/* place string in flash so that it doesn't use up RAM. 
*  but then also needs special handling of PROGMEM strings. */
const char used_libs[] PROGMEM = { PLATFORMIO_USED_LIBRARIES };
__FlashStringHelper* used_libs_str = (__FlashStringHelper *)used_libs; 

CBoardsInformation BoardInformation;


// Get the string name of type enum values used in this example
static const char* get_type_str(esp_partition_type_t type)
{
    switch(type) {
        case ESP_PARTITION_TYPE_APP:
            return "ESP_PARTITION_TYPE_APP";
        case ESP_PARTITION_TYPE_DATA:
            return "ESP_PARTITION_TYPE_DATA";
        default:
            return "UNKNOWN_PARTITION_TYPE"; // type not used in this example
    }
}

// Get the string name of subtype enum values used in this example
static const char* get_subtype_str(esp_partition_subtype_t subtype)
{
    switch(subtype) {
        case ESP_PARTITION_SUBTYPE_DATA_NVS:
            return "ESP_PARTITION_SUBTYPE_DATA_NVS";
        case ESP_PARTITION_SUBTYPE_DATA_PHY:
            return "ESP_PARTITION_SUBTYPE_DATA_PHY";
        case ESP_PARTITION_SUBTYPE_APP_FACTORY:
            return "ESP_PARTITION_SUBTYPE_APP_FACTORY";
        case ESP_PARTITION_SUBTYPE_DATA_FAT:
            return "ESP_PARTITION_SUBTYPE_DATA_FAT";
        default:
            return "UNKNOWN_PARTITION_SUBTYPE"; // subtype not used in this example
    }
}


static void find_partition(const char* comment,esp_partition_type_t type, esp_partition_subtype_t subtype, const char* name)
{
    // Serial.printf( "Find partition with type %s, subtype %s, label %s...\n", get_type_str(type), get_subtype_str(subtype), name == NULL ? "NULL (unspecified)" : name);
    const esp_partition_t * part  = esp_partition_find_first(type, subtype, name);
    if (part != NULL) {
        Serial.printf("%s found! Label '%s' at offset 0x%x with size %d byte\n", comment,part->label, part->address, part->size);
    } else {
        Serial.printf("%s not found!\n",comment);
    }
}

void CBoardsInformation::PrintBoardInformation()
{

  // Standardpartition 1,3MB Flash: board_build.partitions = default.csv
  // MinSPIFFS max App 1,9MB Flash: board_build.partitions = min_spiffs.csv

  Serial.printf("*************************************************************************************************************\n");
  Serial.printf("Boardinformation OTA Version\n");
  Serial.printf("*************************************************************************************************************\n");
  Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("This chip has %d cores\n", ESP.getChipCores());
  Serial.printf("PSRam Size = %d\n", ESP.getPsramSize());
  Serial.printf("Heap Size = %d\n", ESP.getHeapSize());
  Serial.printf("Free Heap Size = %d\n", ESP.getFreeHeap());
  Serial.printf("Flash Chip Size = %d\n", ESP.getFlashChipSize());
  Serial.printf("Flash Chip Speed = %d\n", ESP.getFlashChipSpeed());
  Serial.printf("Flash Chip Mode = %d\n", ESP.getFlashChipMode());
  Serial.printf("CPU Frequency = %d MHZ\n", ESP.getCpuFreqMHz());
  Serial.printf("SDK Version = %s\n", ESP.getSdkVersion());
  Serial.printf("Sketch Size = %d\n", ESP.getSketchSize());
  Serial.printf("Free Sketch Space = %d\n", ESP.getFreeSketchSpace());
  Serial.printf("\n");
  Serial.printf("List of required Mainpartitions\n");
  find_partition("System NVS Partition",ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);
  find_partition("System OTA Partition",ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, NULL);
  find_partition("OTA/App Partition",ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
  find_partition("OTA/App Partition",ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
  find_partition("Filesystem Partition",ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
  Serial.printf("*************************************************************************************************************\n");
 
}
/* prints the string that contains all libraries. doesn't require you to know the specific macros. */
void CBoardsInformation::print_used_libraries() {
    Serial.println(F("***************************************************"));
    Serial.printf("SDK Version:%s\n",ESP.getSdkVersion());
    Serial.printf("Framework_Adruino_Espressif32:%d.%d.%d\n",ESP_ARDUINO_VERSION_MAJOR,ESP_ARDUINO_VERSION_MINOR,ESP_ARDUINO_VERSION_PATCH);//ESP_ARDUINO_VERSION);//PIO_PACKAGE_FRAMEWORK_ARDUINOESPRESSIF32_DECODED_VERSION);
    Serial.printf("Platform:%s\n",PIO_PLATFORM_VERSION);
    Serial.println(F("Verwendete Libraries (PLATFORMIO_USED_LIBRARIES)"));
    // Serial.printf("SDK Version lowlevel:%s\n",esp_get_idf_version());
    Serial.println(used_libs_str);
    Serial.println(F("***************************************************"));
    // Serial.println(F("Framework:"));
    // Serial.println(F(PIO_PACKAGE_FRAMEWORK_ARDUINOESPRESSIF32_PKG_VERSION));
    // Serial.println(F(PIO_PLATFORM_VERSION_MINOR));
    // Serial.println(F(PIO_PLATFORM_VERSION_RELEASE));
    // Serial.printf("Platform:%s.%s.%s\n",PIO_PLATFORM_VERSION_MAJOR,PIO_PLATFORM_VERSION_MINOR,PIO_PLATFORM_VERSION_RELEASE);

    /* you can search through the string per strchr_P() functions etc */
    m_strPlatformVersion =PIO_PLATFORM_VERSION;
	m_strFrameworkVersion =String(ESP_ARDUINO_VERSION_MAJOR)+"."+String(ESP_ARDUINO_VERSION_MINOR)+"."+String(ESP_ARDUINO_VERSION_PATCH);//PIO_PACKAGE_FRAMEWORK_ARDUINOESPRESSIF32_DECODED_VERSION;
    m_strSDKVersion=ESP.getSdkVersion();
	m_strLibraryVersion=used_libs_str;
}

void CBoardsInformation::SendLibraryVersion( WebserverLibraryInfo LibraryVersionInfo)
{
        Serial.printf("SendLibraryVersion to Webserver\n");

        String JSON;
        // StaticJsonDocument<5000> jsonBuffer;
        FirebaseJson jsonBuffer;
        jsonBuffer.set("firmwareurl",String(configManager.data.FirmwareURL)); 
        jsonBuffer.set("platformversion",BoardInformation.m_strPlatformVersion); 
        jsonBuffer.set("frameworkversion",BoardInformation.m_strFrameworkVersion); 
        jsonBuffer.set("sdkversion",BoardInformation.m_strSDKVersion); 
        jsonBuffer.set("libversion",BoardInformation.m_strLibraryVersion); 
        JSON = jsonBuffer.raw();
        Serial.printf("api/firmwareurl/get:%s<-Ende\n",JSON.c_str());
        LibraryVersionInfo.request->send(200, PSTR("text/html"), JSON);
}

/* prints version of a library that we know is included. */
// void print_known_libray() {
//     //again uses F() to create a flash-string to save space on the poor Uno..
//     Serial.print(F("Adafruit GFX library version: "));
//     Serial.println(F(LIB_VERSION_ADAFRUIT_GFX_LIBRARY));
// }
