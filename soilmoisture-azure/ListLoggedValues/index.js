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
    The values from the Capacitive Soil Moisture Sensor v1.2 are roughly as follows when the soil is old (tightly packed), batteries are fresh and the sensor is new:
        730: dry air
        630: very dry soil
        330: very wet soil (just watered)
        0-1: no sensor connected

    When batteries start to wear out, the values go down, eventually staying below 450.
    With a new soil that isn't very tightly packed, the values are lower also, around the following:
        600: dry air
        500: very dry soil

    Hence with an old soil values between 330-630 are good.
    With a fresh soil values between 300-500 are good.
    */

    // For lower limit 300 seems to be a good enough estimate regardless of soil.
    const capacitance_limit = process.env["CapacitanceLimit"] || 600
    const moisture_percent = Math.round(Math.max(0, Math.min(300, (capacitance_limit - logItem.value))) / 3);
    logItem["moisturePercent"] = moisture_percent;

    const status = "moistureStatus";
    if (logItem.value < 10) {
        logItem[status] = "N/A: No sensor connected";
    } else if (logItem.value < 200) {
        logItem[status] = "N/A: Test value";
    } else if (logItem.value > 620) {
        logItem[status] = "N/A: Not in soil";
    } else if (logItem.value < 300) {
        logItem[status] = "Watery";
    } else if (moisture_percent > 10) {
        logItem[status] = "OK";
    } else {
        // Sent notification at least when this is logged if not earlier.
        logItem[status] = "Very dry";
    }
    return logItem;
}
