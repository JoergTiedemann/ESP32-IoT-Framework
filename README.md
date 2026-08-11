# ESP32 IoT Framework ![Status](https://travis-ci.com/maakbaas/esp8266-iot-framework.svg?branch=master)
<p><strong>
This is a fork from the famous ESP8266-IoT-Library from maakbas: <BR>
<a href="https://github.com/maakbaas/esp8266-iot-framework">maakbaas/esp8266-iot-framework</a><BR>
All rights from maakbaas are reserved !<BR>
Target of the actual development is to make the library useable for ESP32 systems
</p></strong>  

The ESP32 IoT Framework is a set of modules to be used as a starting point in new ESP32 projects, implementing HTTPS requests, a React web interface, WiFi manager, configuration manager, live dashboard, NVS support, OTA updates and diagnostic manager.

The unique advantage of this framework is that code generation at build time is used to provide different benefits. Code generation is used to dynamically generate a configuration struct and a live dashboard from JSON files, to incorporate the web interface into PROGMEM in the firmware.

## Documentation
For documentation please see [original documentation](https://github.com/maakbaas/esp8266-iot-framework#introduction)


### Changes to original version from maakbas
* SSL certificate storeage is removed
* standard esp32 timesync library is used
* standard esp32 littlefs filesystem is used
* option to use over the air upodates just by a download url to prevent downloading firmware to file system befor OTA
* diagnostic manager added to monitor variables and logging information via html website
* nvs manager for support of non volaitile storage of data added
* checkbox in dashboard page as on/off switch


## New features in dashboard and configuration page 

### Outline of dashboard and configuration page 
* for headers:
"type": "header",
"text": Uberschriftentext"

* for lable:
"type": "label",
"text" : "labeltext"

* for separator lines: 
 "type" : "separator"

### Double button (2 buttons in a single row on the dashboard)
To define two buttons in a single row on the dashboard (effectively a "dual button"), you need two variables in the dashboard data; each is set to `true` when the corresponding button is pressed.
To do this, first define the dual button with the appropriate labels and the name for the left "On" button. Then, as a second element, define a boolean with the `hidden` property—placed immediately after the first—containing the name for the right "Off" button.
When one of the buttons is pressed, the dashboard data will reflect this by setting either the variable with the "On" name or the one with the "Off" name to `true`.
```
 {
        "name": "ButtonON",
        "label": "Test Double Button",
        "type": "bool",
        "hidden":false,
        "direction": "control",
        "buttontextOn": "einschalten",
        "buttontextOff": "ausschalten",
        "display": "doublebutton"
    },
    {
        "name": "ButtonOFF",
        "type": "bool",
        "hidden":true
    },
```

### Combobox control in dashboard and configuration page
To define a combobox control in dashboard or configuration page use the following code
```
  {
        "name": "Messagelevel",
        "label": "Meldungsfilter",
        "type": "uint16_t",
        "value": 2,
        "control": "select",
        "options": [
            {"Label":"Fehler+Meldungen","Value":2},
            {"Label":"nur Fehler","Value":1}
        ]
    },    
```

## Requirements
This Library requires adruino espressif framework 3.3.9 from the pioarduino project  
For the webserver the html code is base on react and node modules  
You have to install the node modules manually.  
In the directroy .pio\libdeps\esp32dev\ESP32-IoT-Framework execute the following comand 
```
npm install 
```
to install the required node packages


## Quick start
If you are new to PlatformIO, start with the [installation guide](https://github.com/maakbaas/esp8266-iot-framework/blob/master/docs/installation-guide.md). Otherwise, simply start a new project for your ESP32, and add the following line to your `platformio.ini` file:
```
joergtiedemann/ESP32-IoT-Framework@^1.0.0
```

Take one of the [examples](https://github.com/maakbaas/esp8266-iot-framework/tree/master/examples) as a starting point to develop your application.

