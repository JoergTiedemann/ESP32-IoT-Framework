import React from "react";
import PropTypes from "prop-types";

import "../../../node_modules/react-vis/dist/style.css";
import { FlexibleWidthXYPlot, XAxis, YAxis, HorizontalGridLines, VerticalGridLines, LineSeries,VerticalBarSeries,LabelSeries } from "react-vis";

export function DisplayItem(props) {

    if (props.item.display == "graph") {
        // console.log("DisplayItems  Anzahl ArrItems:",props.Arrdata.length);
        // for (let i = 0; i < props.Arrdata.length; i++) 
        // {
        //     let parsedData = props.Arrdata[i];
        //     console.log("Index:",i,"Timestamp:",parsedData["timestamp"],"Wert:",parsedData["value"]);
        // }

        const chartData = [];
        let min;
        let max;
        let xLim = [0, 1];
        if (props.data.length > 0) {
            min = props.data[0][1][props.item.name];
            max = props.data[0][1][props.item.name];

            xLim = [props.data[props.data.length - 1][0] - (props.item.xaxis || 10) * 1000 + 500, props.data[props.data.length - 1][0] - 500];
        }
        for (let j = 0; j < props.data.length; j++) {
            if (props.data[j][0] > props.data[props.data.length - 1][0] - (props.item.xaxis || 10) * 1000 + 500) {
                chartData.push({ x: props.data[j][0], y: props.data[j][1][props.item.name] });
                min = Math.min(min, props.data[j][1][props.item.name]);
                max = Math.max(max, props.data[j][1][props.item.name]);
            }
        }

        return <div>
            <FlexibleWidthXYPlot
                xDomain={xLim}
                margin={{ left: 40, right: 10, top: 10, bottom: 10 }}
                xType='time'
                yDomain={[min - 0.1 * (max - min), max + 0.1 * (max - min)]}
                yBaseValue={min - 0.1 * (max - min)}
                height={150}>
                <HorizontalGridLines />
                <VerticalGridLines />
                <XAxis
                    tickValues={[]} />
                <YAxis
                    tickSizeInner={0} />
                <LineSeries
                    color="#000"
                    data={chartData} />
            </FlexibleWidthXYPlot>
        </div>;

    } 
    else 
    {
        //  console.log("Barchart Laenge:",props.Arrdata.length);
        if (props.item.display == "barchart") {
            //  console.log("DisplayItems Props  :",props);

            // console.log("DisplayItems  Anzahl ArrItems:",props.Arrdata.length);
            // for (let i = 0; i < props.Arrdata.length; i++) 
            // {
            //     let parsedData = props.Arrdata[i];
            //     console.log("Index:",i,"Timestamp:",parsedData["timestamp"],"Wert:",parsedData["value"]);
            // }

            const chartData = [];
            let min;
            let max = 0;
            let xLim = [0, 1];
            if ((props.item.type == "historic") || (props.item.type == "historic1"))
            {
                for (let j = 0; j < props.Arrdata.length; j++) {
                    
                    chartData.push({ x: props.Arrdata[j]["timestamp"], y: props.Arrdata[j]["value"] });
                    // min = Math.min(min, props.data[j][1][props.item.name]);
                    max = Math.max(max, props.Arrdata[j]["value"]);
                }
            }
            else
            {
                for (let j = 0; j < props.Arrdata2.length; j++) {
                    
                    chartData.push({ x: props.Arrdata2[j]["timestamp"], y: props.Arrdata2[j]["value"] });
                    // min = Math.min(min, props.data[j][1][props.item.name]);
                    max = Math.max(max, props.Arrdata2[j]["value"]);
                }
            }
            if (max < 10)
                max = 10;
                // console.log("max:",max);

        //console.log("Alignment:",props.item.alignment);
        return <div>
                <FlexibleWidthXYPlot
                    // xDomain={xLim}
                    margin={{ left: 40, right: 10, top: 10, bottom: props.item.alignment == "vertical" ? 55 : 25 }}
                    xType="ordinal"
                    //yDomain={[0,1000]}
                    yDomain={[0,max+(max*0.1)]}
                    xDistance={100}
                    // yBaseValue={min - 0.1 * (max - min)}
                    height={300}>
                    <HorizontalGridLines />
                    <VerticalGridLines />
                    <XAxis tickLabelAngle={props.item.alignment == "vertical" ? -90 : 0}/>
                    <YAxis/>
                    <VerticalBarSeries
                        // color="#000"
                        data={chartData} />
                    <LabelSeries
                    data = {chartData.map(obj => {
                        return { ...obj,label:obj.y.toString()}
                    })}
                    labelAnchorX="middle"
                    labelAnchorY="text-after-edge"
                    />
                </FlexibleWidthXYPlot>
            </div>;

        }
        else {
            if (props.item.type == "bool") 
            {
                // erstmal den Text besetzen            
                let btxt = props.value.toString();
                if (props.value.toString() == "true") 
                {
                    if (props.item.texttrue === undefined)
                        btxt = "true";
                    else
                        btxt = props.item.texttrue;
                }
                else
                {
                    if (props.item.textfalse === undefined)
                        btxt = "false";
                    else
                        btxt = props.item.textfalse;
                }
                // und nun das Aussehen bzw. den Stil
                let clsname;
                if (props.value.toString() == "true")
                {
                    if (props.item.styletrue === undefined)
                        clsname = "true";
                    else
                        clsname = props.item.styletrue;
                }
                else
                {
                    if (props.item.stylefalse === undefined)
                        clsname = "false";
                    else
                        clsname = props.item.stylefalse;
                }
                return <span id={props.item.name} name={props.item.name} className={clsname}>{btxt}</span>;
            }
            else
               return <span id={props.item.name} name={props.item.name} className={props.item.type == "bool" ? props.value.toString() : ""}>{props.value.toString()}</span>;
        }
    }
}

DisplayItem.propTypes = {
    item: PropTypes.array,
    value: PropTypes.any,
    Arrdata: PropTypes.array,
    Arrdata2: PropTypes.array,
};