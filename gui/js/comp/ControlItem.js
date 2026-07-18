import React, { useState, useEffect } from "react";
import PropTypes from "prop-types";

import { Button } from "./UiComponents";
import { FiUpload as Upload } from "react-icons/fi";

import Dash from "../dashboard.json";
import { binsize } from "./../functions/configHelpers";
import Toggle from 'react-toggle'
import "react-toggle/style.css" // 
let btnName;

export function ControlItem(props) {

    const [data, setData] = useState([]);
    const [target, setTarget] = useState(null);
    const [saving, setSaving] = useState(false);
    const [saved, setSaved] = useState(false);

    //populate graphs
    useEffect(() => {
        if (target != null) {
            setData(target);                  
        } else {
            setData(props.value);                  
        }

        if (saved && props.value == target) {
            setSaving(0);
            setSaved(0);
            setTarget(null);
        }

        if (saving && target != null) {
            btnName = props.name;
            if (props.displayType == "doublebutton") {
                if (target === false)
                    btnName = props.nameOffbtn;
            }
            // console.log("Name:",props.name," OffbtnName",props.nameOffbtn,"Distyp:",props.displayType," btnName:",btnName);

            const sizes = binsize(btnName, Dash);
            const binData = new ArrayBuffer(sizes[1]);
            const binDataView = new DataView(binData);
            switch (props.dataType) {
                case "char":
                    for (let j = 0; j < target.length; j++) {
                        binDataView.setUint8(j, (new TextEncoder).encode(target[j])[0]);
                    }
                    binDataView.setUint8(target.length, 0);
                    break;
                case "bool":
                    if (props.displayType == "doublebutton") 
                    {
                        binDataView.setUint8(0, 1);
                    }
                    else 
                    {
                        if (target === true) { binDataView.setUint8(0, 1); } else { binDataView.setUint8(0, 0); }
                    }
                    break;
                case "uint8_t":
                    binDataView.setUint8(0, Number(target));
                    break;
                case "int8_t":
                    binDataView.setInt8(0, Number(target));
                    break;
                case "uint16_t":
                    binDataView.setUint16(0, Number(target), true);
                    break;
                case "int16_t":
                    binDataView.setInt16(0, Number(target), true);
                    break;
                case "uint32_t":
                    binDataView.setUint32(0, Number(target), true);
                    break;
                case "int32_t":
                    binDataView.setInt32(0, Number(target), true);
                    break;
                case "float":
                    binDataView.setFloat32(0, Number(target), true);
                    break;
            }
            // if  (props.displayType == "doublebutton")
            //     console.log("Name:",btnName," sizes:",sizes," binData:",binData," bindDataView:",binDataView);

            fetch(`${props.API}/api/dash/set?start=${sizes[0]}&length=${sizes[1]}`, {
                method: "post",
                body: binData,
            });  
            setSaved(true);
            if  ((props.displayType == "doublebutton") ||
                 (props.displayType == "button")
                )
           {
            // hierdruch wird sichergestellt das das Senden bei Buttons nur einmal durchgeführt wird weil 
            // der useeffect u.U. mehrfach mit dem gleichen Parametern aufgerufen wird und dann das Buttonereignis 
            // mehrfach an den Server uebermittelt wird was wir nicht wollen
            // console.log("Fertig:",saving,"Name:",props.name," target Wert:",target," props value:",props.value);
            setSaving(0);
            setSaved(0);
            setTarget(null);
            }
        }
    });

    function save() {
        setSaving(1);
    }

    let savebtn;
    let checkbox = false;

    if (typeof props.conditionalAttributes !== "undefined" && typeof props.conditionalAttributes.checked !== "undefined") {
        props.conditionalAttributes.checked = data;
        checkbox = true;
    } else {
        savebtn = <Button onClick={(e) => {            
            e.preventDefault();
            console.log(" Savebutton Save()");
            save();
        }}><Upload /></Button>;
    }

    // console.log(" Name:",props.name, " Type:",props.type," Display:",props.displayType);
    if (props.type == "select") {
        let options;
        let isOption = false;
        for (let i = 0; i < props.conditionalAttributes.options.length; i++) {
            if (data == props.conditionalAttributes.options[i].Value) {
                isOption = true;
            }             
            options = <>{options}<option value={props.conditionalAttributes.options[i].Value}>{props.conditionalAttributes.options[i].Label}</option></>;            
        }

        if (!isOption) {
            options = <><option value={data}>{data}</option>{options}</>;
        }

        return <select id={props.name} name={props.name} value={data} onChange={(e) => { setTarget(e.target.value); save(); }}>
            {options}
        </select>;
    }
    if ((props.type == "checkbox") && (props.displayType == "switch"))
    {
        //console.log("ToggleSwitch");
         return <><Toggle onChange={(e) => {setTarget(e.target.checked); save(); }} defaultChecked ={props.conditionalAttributes.checked} type={props.type} id={props.name} name={props.name} {...props.conditionalAttributes} />{savebtn}</>;
    }
   else
   {
    if ((props.type == "checkbox") && ((props.displayType == "button") || (props.displayType == "doublebutton")))
        {
            //console.log("Button");
            //<Button onClick={(e) => {e.preventDefault();e.target.checked=1;setTarget(e.target.checked);save(); }} type={props.type} id={props.name} name={props.name}>{props.conditionalAttributes.buttontextOn}</Button>
            if (props.displayType == "doublebutton")
            {
                // console.log("DoubleButton IdOn:",props.conditionalAttributes.buttonIdOn," IdOff:",props.conditionalAttributes.buttonIdOff," name:",props.name," NameOffbtn:",props.nameOffbtn);
                // 2fach Button
                /*
                return <>
                <Button onClick={(e) => {e.preventDefault();setTarget(true);save(); }} type={props.type} id={props.conditionalAttributes.buttonIdOn} name={props.name}>{props.conditionalAttributes.buttontextOn}</Button>
                &nbsp;
                <Button onClick={(e) => {e.preventDefault();setTarget(false);save(); }} type={props.type} id={props.conditionalAttributes.buttonIdOff} name={props.name}>{props.conditionalAttributes.buttontextOff}</Button>
                </>;
                */
                return <>
                <Button onClick={(e) => {e.preventDefault();setTarget(true);save(); }} type={props.type} id={props.conditionalAttributes.buttonIdOn} name={props.name}>{props.conditionalAttributes.buttontextOn}</Button>
                &nbsp;
                <Button onClick={(e) => {e.preventDefault();setTarget(false);save(); }} type={props.type} id={props.conditionalAttributes.buttonIdOff} name={props.name}>{props.conditionalAttributes.buttontextOff}</Button>
                </>;

}
            else
            {
                // 1fach Button
                return <>
                <Button onClick={(e) => {e.preventDefault();setTarget(true);save(); }} type={props.type} id={props.conditionalAttributes.buttonIdOn} name={props.name}>{props.conditionalAttributes.buttontextOn}</Button>
                </>;
            }
        }
    else
    {
        // console.log("Standard");
        return <><input onChange={(e) => { if (checkbox) { setTarget(e.target.checked); save(); } else { setTarget(e.target.value); } }} type={props.type} id={props.name} name={props.name} value={data} {...props.conditionalAttributes} />{savebtn}</>;
    }
   }
    
//    return <><input onChange={(e) => { if (checkbox) { setTarget(e.target.checked); save(); } else { setTarget(e.target.value); } }} type={props.type} id={props.name} name={props.name} value={data} {...props.conditionalAttributes} />{savebtn}</>;
}

ControlItem.propTypes = {
    type: PropTypes.string,
    API: PropTypes.string,
    id: PropTypes.string,
    name: PropTypes.string,
    value: PropTypes.any,
    nameOffbtn: PropTypes.string,
    valueOffbtn: PropTypes.any,
    sizes: PropTypes.array,
    dataType: PropTypes.string,
    displayType: PropTypes.string,
    conditionalAttributes: PropTypes.object,
};
