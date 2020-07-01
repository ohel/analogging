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
    The values from the Capacitive Soil Moisture Sensor v1.2 are roughly as follows when brand new:
        730: dry air
        630: very dry soil
        330: very wet soil (just watered)
        0-1: no sensor connected

    After a year in use, the values have dropped to:
        600: dry air
        500: very dry soil
        200: water

    The soil also affects the values. With the original soil values between 330-630 were good.
    After changing to new soil the values usually fall between 290-480.
    */

    logItem["moisturePercent"] = Math.round(Math.max(0, Math.min(300, (500 - logItem.value))) / 3);

    const status = "moistureStatus";
    if (logItem.value < 10) {
        logItem[status] = "N/A: No sensor connected";
    } else if (logItem.value < 200) {
        logItem[status] = "N/A: Test value";
    } else if (logItem.value >= 600) {
        logItem[status] = "N/A: Not in soil";
    } else if (logItem.value < 300) {
        logItem[status] = "Moist";
    } else if (logItem.value < 400) {
        logItem[status] = "OK";
    } else if (logItem.value < 500) {
        logItem[status] = "Dry";
    } else {
        // Sent notification at least when this is logged if not earlier.
        logItem[status] = "Very dry";
    }
    return logItem;
}
