import React, { useEffect } from "react";
import PropTypes from "prop-types";

import { DiagListing } from "./DiagListing";  

import Config from "./../configuration.json";
let loc;
if (Config.find(entry => entry.name === "language")) {
    loc = require("./../lang/" + Config.find(entry => entry.name === "language").value + ".json");
} else {
    loc = require("./../lang/en.json");
}

export function DiagPage(props) {

   useEffect(() => {
        document.title = loc.titleDiagnostic;
    }, []);

    return <><h2>{loc.titleDiagnostic}</h2><DiagListing API={props.API} /></>;

}

DiagPage.propTypes = {
    API: PropTypes.string,
};

