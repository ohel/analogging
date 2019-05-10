module.exports = function (context, req) {
    const table = context.bindings.moistureValues;

    const values = table.map(row => {
        return {"timestamp": row.RowKey, "value": row.Value}
    });

    values.map(valueToMoisture);

    context.res.status(200).json(values.sort(latestFirst));
    context.done();
};

function latestFirst(a, b) {
    return (a["timestamp"] > b["timestamp"] ? -1 : 1);
}

function valueToMoisture(logItem) {
    /*
    The values from the Capacitive Soil Moisture Sensor v1.2 are roughly as follows:
        730: dry air
        630: very dry soil
        330: very wet soil (just watered)
        0-1: no sensor connected

    Therefore we consider 630..330 to be the usable scale.
    */
    logItem["moisturePercent"] = Math.round(Math.max(0, Math.min(300, (630 - logItem.value))) / 3);

    const status = "moistureStatus";
    if (logItem.value < 10) {
        logItem[status] = "N/A: No sensor connected";
    } else if (logItem.value < 300) {
        logItem[status] = "N/A: Test value";
    } else if (logItem.value > 700) {
        logItem[status] = "N/A: Not in soil";
    } else if (logItem.value < 400) {
        logItem[status] = "Moist";
    } else if (logItem.value < 580) {
        logItem[status] = "OK";
    } else if (logItem.value < 600) {
        logItem[status] = "Dry";
    } else {
        // Sent notification when this is logged.
        logItem[status] = "Very dry";
    }
    return logItem;
}
