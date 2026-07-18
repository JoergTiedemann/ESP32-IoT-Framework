import React, { useEffect } from "react";
import PropTypes from "prop-types";
import { FirmwareListing } from "./FirmwareListing";  


import Config from "./../configuration.json";
let loc;
if (Config.find(entry => entry.name === "language")) {
    loc = require("./../lang/" + Config.find(entry => entry.name === "language").value + ".json");
} else {
    loc = require("./../lang/en.json");
}

export function WebFirmwarePage(props) {

    useEffect(() => {
        document.title = loc.titleFirmwareUpdate;
    }, []);

    return <><h2>{loc.titleFirmwareUpdate}</h2><FirmwareListing API={props.API} /></>;
    
}

WebFirmwarePage.propTypes = {
    API: PropTypes.string,
};

