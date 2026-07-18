import React, { useState, useEffect } from "react";
import ReactDOM from "react-dom";
import {BrowserRouter, Switch, Route, NavLink} from "react-router-dom";
import { FiBox as HeaderIcon } from "react-icons/fi";


import {GlobalStyle, Menu, Header, Page, Hamburger} from "./comp/UiComponents";
import { WifiPage } from "./comp/WifiPage";
import { ConfigPage } from "./comp/ConfigPage";
import { DashboardPage } from "./comp/DashboardPage";
import { FilePage } from "./comp/FilePage";
import { DiagPage } from "./comp/DiagPage";
import { FirmwarePage } from "./comp/FirmwarePage";
import { WebFirmwarePage } from "./comp/WebFirmwarePage";

import { bin2obj } from "./functions/configHelpers";

import Config from "./configuration.json";
import Dash from "./dashboard.json";
import HistoricData from "./HistoricData.json";


let loc;
if (Config.find(entry => entry.name === "language")) {
    loc = require("./lang/" + Config.find(entry => entry.name === "language").value + ".json");
} else {
    loc = require("./lang/en.json");
}


let url = "http://192.168.178.63";
if (process.env.NODE_ENV === "production") {url = window.location.origin;}

if (process.env.NODE_ENV === "development") {require("preact/debug");}

const displayData = new Array();
const historicArrayData1 = new Array();
const historicArrayData2 = new Array();


function Root() {
    
    const [menu, setMenu] = useState(false);
    const [configData, setConfigData] = useState(new Object());
    const [binSize, setBinSize] = useState(0);
    const [socket, setSocket] = useState({});

    useEffect(() => {
        const ws = new WebSocket(url.replace("http://","ws://").concat("/ws"));
        ws.addEventListener("message", wsMessage);
        setSocket(ws);
        fetchData();        
    }, []);

    function wsMessage(event) {
        event.data.arrayBuffer().then((buffer) => {                
            const dv = new DataView(buffer, 0);
            // als 1.  der Timestamp
            const timestamp = dv.getUint32(0, true);
            // als 2. die Laenge der Dashboarddatenstruktur 
            const dashsize   = dv.getUint32(4, true);
            // als 3. die Laenge des Arrays fuer historische Daten (dieses kommt aber 2mal)
            const historicArraylength  = dv.getUint32(4+4, true);
            // nun die eigentlichen Dashdaten 
            displayData.push([timestamp, bin2obj(buffer.slice(12,12+dashsize), Dash)]);     
            // nun die historischen Arraydaten  
            if (historicArraylength > 0)
            {
                const ArrElementlength = (buffer.byteLength-12-dashsize) / (historicArraylength*2);
                // console.log("ArrElementlength:",ArrElementlength,"HistoricArraylength:",historicArraylength);
                // erstmal alles loeschen
                historicArrayData1.length = 0;
                historicArrayData2.length = 0;
                for (let i = 0; i < historicArraylength; i++) 
                {
                    historicArrayData1.push(bin2obj(buffer.slice(12+dashsize+i*ArrElementlength,12+dashsize+(i+1)*ArrElementlength), HistoricData));     
                    historicArrayData2.push(bin2obj(buffer.slice(12+dashsize+(historicArraylength*ArrElementlength)+i*ArrElementlength,12+dashsize+(historicArraylength*ArrElementlength)+(i+1)*ArrElementlength), HistoricData));     
                }
                //  console.log("Arrayinhalt Array 1");
                //  for (let i = 0; i < historicArraylength; i++) 
                //  {
                //      let parsedData = historicArrayData1[i];
                //      console.log("Index:",i,"Timestamp:",parsedData["timestamp"],"Wert:",parsedData["value"]);
                //  }
                //  console.log("Arrayinhalt Array 2");
                //  console.log("dashsize:",dashsize);
                //  console.log("bin2obj 0:",bin2obj(buffer.slice(24+dashsize+(historicArraylength*ArrElementlength),24+dashsize+(historicArraylength*ArrElementlength)+ArrElementlength), HistoricData));
                //  for (let i = 0; i < historicArraylength; i++) 
                //  {
                //      let parsedData = historicArrayData2[i];
                //      console.log("Index:",i,"Timestamp:",parsedData["timestamp"],"Wert:",parsedData["value"]);
                //  }
           }
            // console.log("wsMessage Anzahl ArrItems:",historicArrayData.length);

            //displayData.push([timestamp, bin2obj(buffer.slice(8,buffer.byteLength), Dash)]);     
        });        
    }

    function fetchData() {
        fetch(`${url}/api/config/get`)
            .then((response) => {
                return response.arrayBuffer();
            })
            .then((data) => {
                setBinSize(data.byteLength);
                setConfigData(bin2obj(data, Config));
            });
    }

    const projectName = configData["projectName"] || Config.find(entry => entry.name === "projectName") ? Config.find(entry => entry.name === "projectName").value : "ESP8266";
    const projectVersion = configData["projectVersion"] || Config.find(entry => entry.name === "projectVersion") ? Config.find(entry => entry.name === "projectVersion").value : "";
    
    // Filepage anzeigen aus Konfiguration 
    let showFilePage;
    showFilePage = true;
    if (Config.find(entry => entry.name === "ShowFilePage")){
        showFilePage = configData["ShowFilePage"];
    }
    let fileentry;
    if (showFilePage){
        fileentry =  <li><NavLink onClick={() => setMenu(false)} exact to="/files">{loc.titleFile}</NavLink></li>;
    }

    // FileFirmwarepage anzeigen aus Konfiguration 
    let showFileFirmwarePage;
    showFileFirmwarePage = true;
    if (Config.find(entry => entry.name === "ShowFileFirmwarePage")){
        showFileFirmwarePage = configData["ShowFileFirmwarePage"];
    }

    let filefirmwareentry;
    if (showFileFirmwarePage){
        filefirmwareentry =  <li><NavLink onClick={() => setMenu(false)} exact to="/firmware">{loc.titleFw}</NavLink></li>;
    }

    // WebFirmwarepage anzeigen aus Konfiguration 
    let showWebFirmwarePage;
    showWebFirmwarePage = true;
    if (Config.find(entry => entry.name === "ShowWebFirmwarePage")){
        showWebFirmwarePage = configData["ShowWebFirmwarePage"];
    }
    let webfirmwareentry;
    if (showWebFirmwarePage){
        webfirmwareentry =  <li><NavLink onClick={() => setMenu(false)} exact to="/webfirmware">{loc.titleFw}</NavLink></li>;
    }

    // Diagnosepage anzeigen aus Konfiguration 
    let showDiagPage;
    showDiagPage = true;
    if (Config.find(entry => entry.name === "ShowDiagnosePage")){
        showDiagPage = configData["ShowDiagnosePage"];
    }
    let diagentry;
    if (showDiagPage){
        diagentry = <li><NavLink onClick={() => setMenu(false)} exact to="/diag">{loc.titleDiagnostic}</NavLink></li>;
    }


return <><GlobalStyle />

        <BrowserRouter>

            <Header>
                <h1><HeaderIcon style={{verticalAlign:"-0.1em"}} /> {projectName} {projectVersion}</h1>

                <Hamburger onClick={() => setMenu(!menu)} />
                <Menu className={menu ? "" : "menuHidden"}>
                    <li><NavLink onClick={() => setMenu(false)} exact to="/">{loc.titleDash}</NavLink></li>
                    <li><NavLink onClick={() => setMenu(false)} exact to="/config">{loc.titleConf}</NavLink></li>
                    <li><NavLink onClick={() => setMenu(false)} exact to="/Wifi">{loc.titleWifi}</NavLink></li>
                    {fileentry}
                    {diagentry}
                    {filefirmwareentry}
                    {webfirmwareentry}
                </Menu>

            </Header>
        
            <Page>
                <Switch>
                    <Route exact path="/files">
                        <FilePage API={url} />
                    </Route>
                    <Route exact path="/diag">
                        <DiagPage API={url} />
                    </Route>
                    <Route exact path="/config">
                        <ConfigPage API={url} 
                            configData={configData}
                            binSize={binSize}
                            requestUpdate={fetchData} />
                    </Route>
                    <Route exact path="/">
                        <DashboardPage API={url} 
                            socket={socket}
                            requestData={() => {return displayData;}} 
                            requestArrData={() => {return historicArrayData1;}} 
                            requestArrData2={() => {return historicArrayData2;}} 
                            />
                    </Route>
                    <Route exact path="/webfirmware">
                        <WebFirmwarePage API={url} />
                    </Route>
                    <Route exact path="/firmware">
                        <FirmwarePage API={url} />
                    </Route>
                    <Route path="/Wifi">
                        <WifiPage API={url} />
                    </Route>
                </Switch>
            </Page>

        </BrowserRouter>
    </>;

}



ReactDOM.render(<Root />, document.getElementById("root"));