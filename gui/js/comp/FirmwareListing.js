import React, { useState, useEffect, version } from "react";
import PropTypes from "prop-types";

import styled from "styled-components";

import { Fetch, Flex, RedButton, Button, buttonStyle, cPrimary, Alert,Confirmation, Spinner } from "./UiComponents";
import { FiFile as File, FiTrash2 as Trash2, FiDownload as Download } from "react-icons/fi";

import Config from "./../configuration.json";
let loc;
if (Config.find(entry => entry.name === "language")) {
    loc = require("./../lang/" + Config.find(entry => entry.name === "language").value + ".json");
} else {
    loc = require("./../lang/en.json");
}
const Layout = styled.div`
    * {
        width:calc(660px + 3em);
        margin-left:0px;
        max-width:calc(100% - 40px);
    }
`;


export function FirmwareListing(props) {
    // const [state, setState] = useState({ firmwareurl: ""});
    const [state, setState] = useState({ firmwareurl: "",platformversion:"",frameworkversion:"",sdkversion:"",libversion:""});
    const [FwUpdate, setFwUpdate] = useState(false);

    useEffect(() => {
        fetchData();
    }, []);

    function fetchData() {
        fetch(`${props.API}/api/firmwareurl/get`)
            .then((response) => {
                return response.json();
            })
            .then((data) => {
                setState(data);
                console.log("webfirmwaredata: Data",data);
            });
            console.log("Firmware Url:",state.firmwareurl);
        }

    function DoFirmwareUpdate() {
        console.log("Do FirrmwareUpdate");
        fetch(`${props.API}/api/firmwareurl/DoUpdate`, { method: "POST" });
        }
    
    
    let header;
    header = loc.firmwareTitle;
    // hier bauen wir uns den HTML code zusammen der für die Aufzaehlung der Libaries am Ende ausgegeben werden soll
    var LibArray = state.libversion.split(",");
    let libItems;
    for (var x = 0; x < LibArray.length; x++) {
        var newstr = LibArray[x].replace(/'/g,"");
        // console.log(newstr);
        var strLib = newstr.split(":");
        libItems = <>{libItems}<Layout>{strLib[0]}:{strLib[1]}<br/></Layout></>;
    }

    //</Flex><h3>{header}</h3>{list}</>;
    // <div><Upload action={`${props.API}/upload`} onFinished={fetchData} filter={props.filter} /></div>
    // return <><h3>{header}</h3></>;

    return <><h3>{header}</h3><Flex>
        {<div>{state.firmwareurl}</div>}
    </Flex>
    <Flex>
         <div><Button onClick={() => { setFwUpdate(true);}}>{loc.firwmwareUpdateBtn}</Button></div> 
     </Flex>
     <><Layout><hr /></Layout></>
     <><Layout><h3>{loc.VersionenTitle}</h3></Layout></>
     <><Layout><p>React (preact 10.24.3):{version}</p></Layout></>
     <><Layout><p>Pioarduino Platformversion (platform.json) :{state.platformversion}</p></Layout></>
     <><Layout><p>Adruino-Espressif-Framework:{state.frameworkversion}</p></Layout></>
     <><Layout><p>ESP-Espressif-SDK:{state.sdkversion}</p></Layout></>
     <><Layout><h4>{loc.LibraryTitle}</h4></Layout></>
     <><Layout>{libItems}</Layout></>
     <Confirmation active={FwUpdate}
    confirm={() =>{setFwUpdate(false);DoFirmwareUpdate();}}
    cancel={() => setFwUpdate(false)}>{loc.firwmwareUpdateQuest}</Confirmation>
     </>; 
    

}

FirmwareListing.propTypes = {
    API: PropTypes.string,
};

