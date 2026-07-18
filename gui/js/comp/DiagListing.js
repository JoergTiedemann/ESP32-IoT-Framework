import React, { useState, useEffect } from "react";
import PropTypes from "prop-types";

import styled from "styled-components";

import { Fetch, Flex, RedButton, Button, buttonStyle, cPrimary, Alert, Confirmation, Spinner } from "./UiComponents";
import { FiCpu as Cpu, FiInfo as Info, FiTrash2 as Trash2, FiDownload as Download } from "react-icons/fi";


import Config from "./../configuration.json";
let loc;
if (Config.find(entry => entry.name === "language")) {
    loc = require("./../lang/" + Config.find(entry => entry.name === "language").value + ".json");
} else {
    loc = require("./../lang/en.json");
}

const DiagLine = styled(Flex)`
    padding:0.35em 0em;
    border-bottom:1px solid #ddd;
    
    :last-child 
    {
        border-bottom:0px;
    }

    &.selectable {
        span {
            text-decoration:underline;
        }
        cursor:pointer;
        padding-left:0.35em;
        padding-right:0.35em;
    }

    &.selectable:hover {
        background-color:#ff00cc11;
    }

    span
    {
        margin-left:0.6em;
    }

    Button
    {
        padding:0.4em 0.5em;        
    }

    div:first-child
    {
        padding:0.4em 0em;
    }

    svg 
    {
        vertical-align: -0.15em;
    }

    @media (max-width: 500px) 
    {
        flex-wrap:wrap;
        div:first-child {
            width:100%;
            margin-bottom:0.4em;
        }
    }
`;

export function DiagListing(props) {
    // const [state, setState] = useState({ files: [], variabletxt1: "",variablevalue1: "",variabletxt2: "",variablevalue2: "",variabletxt3: "",variablevalue3: ""});
    const [state, setState] = useState({ files: [], variablenames: [],variablevalues: []});
    const [currentTime, setcurrentTime] = useState("");
    const [restart, setRestart] = useState(false);


    useEffect(() => {
        fetchData();
    }, []);

    function getDateTime() {
        var now     = new Date(); 
        var year    = now.getFullYear();
        var month   = now.getMonth()+1; 
        var day     = now.getDate();
        var hour    = now.getHours();
        var minute  = now.getMinutes();
        var second  = now.getSeconds(); 
        if(month.toString().length == 1) {
             month = '0'+month;
        }
        if(day.toString().length == 1) {
             day = '0'+day;
        }   
        if(hour.toString().length == 1) {
             hour = '0'+hour;
        }
        if(minute.toString().length == 1) {
             minute = '0'+minute;
        }
        if(second.toString().length == 1) {
             second = '0'+second;
        }   
        // var dateTime = year+'/'+month+'/'+day+' '+hour+':'+minute+':'+second;   
        var dateTime = hour+':'+minute+':'+second;   
         return dateTime;
    }

    function fetchData() {
        fetch(`${props.API}/api/diagnosticdata/get`)
            .then((response) => {
                return response.json();
            })
            .then((data) => {
                setState(data);
                console.log("diagnosticdata: Data",data);
            });
        console.log("Anzahl Files:",state.files.length,"Names:",state.variablenames.length);
        setcurrentTime(getDateTime());
      }
      function RestartMachine() {
        console.log("Restart Machine");
        fetch(`${props.API}/api/restart`, { method: "POST" });
      }
      function DiagDisplay() {
        console.log("Diagdisplayumschaltung");
        fetch(`${props.API}/api/diagdisplay`, { method: "POST" });
      }

      let variablenliste;
      // erst die 3 anzuzeigenen Variablen (wenn denn ein Text definiert wurde
    // if (state.variabletxt1 != ""){
    // variablenliste = <>{variablenliste}                        
    // <DiagLine>
    //     <div>{state.variabletxt1}=<span>{state.variablevalue1}</span></div>
    // </DiagLine></>;
    // }
    // if (state.variabletxt2 != ""){
    // variablenliste = <>{variablenliste}                        
    // <DiagLine>
    //     <div>{state.variabletxt2}=<span>{state.variablevalue2}</span></div>
    // </DiagLine></>;
    // }
    // if (state.variabletxt3 != ""){
    // variablenliste = <>{variablenliste}                        
    // <DiagLine>
    //     <div>{state.variabletxt3}=<span>{state.variablevalue3}</span></div>
    // </DiagLine></>;
    // }
     // nun der Variablenmonitor
    if (state.variablenames.length > 0) {
        for (let i = 0; i < state.variablenames.length; i++) {
            if (state.variablenames[i] != "")
            {
                variablenliste = <>{variablenliste}
                <DiagLine>
                    <div><Cpu/><span>{state.variablenames[i]}=</span><span>{state.variablevalues[i]}</span></div>
                </DiagLine></>;
            }
        }
    }


    // nun die Logbuchliste
    let list;

    if (state.files.length == 0) {
        list = <DiagLine><div>keine Eintraege vorhanden</div></DiagLine>;
    } else {
        for (let i = 0; i < state.files.length; i++) {
                const name = state.files[i];         
                if (name != "")
                {
                    list = <>{list}                        
                        <DiagLine className={props.selectable ? "selectable" : ""} onClick={() => {if (typeof props.onSelect !== "undefined") {props.onSelect(name);}}}>
                            <div><Info/><span>{state.files[i]}</span></div>
                        </DiagLine></>;
                }
            }   
    }  
   
    let variableheader;
    variableheader = loc.diagVarData;

    let logheader;
    logheader = loc.diagLogData;
    
    //console.log("Zeit:",currentTime);
    // und nun die HTML Listen als ein String zurueckgeben
    return <><Flex>
         <div><Button onClick={(e) => {e.preventDefault();fetchData(); }} id="refreshid" name="refreshname">{loc.diagActData}</Button></div> 
         <div>{loc.diagLastActTime}:{currentTime} </div> 
     </Flex>

       {state.variablenames.length > 0 ? <div><h3>{variableheader}</h3> {variablenliste}</div> :""}
     <h3>{logheader}</h3>{list}
     
     <Flex>
         <div><Button onClick={() => { setRestart(true);}}>{loc.fwReboot}</Button></div> 
         <div><Button onClick={(e) => {e.preventDefault();DiagDisplay(); }} id="diagdisplayid" name="diagdisplayname">{loc.DiagDisplay}</Button></div> 
     </Flex>

     <Confirmation active={restart}
    confirm={() =>{setRestart(false);RestartMachine();}}
    cancel={() => setRestart(false)}>{loc.diagResetMachineQuest}</Confirmation>
     </>;
}


DiagListing.propTypes = {
    API: PropTypes.string,
    onSelect: PropTypes.func,
    filter: PropTypes.string,
    selectable: PropTypes.bool,
};

