var https = require("https");

function isNewerThanDeltaLocalizedTimestamp(timestamp, deltaHours, timezoneHourDiff) {
    const timeDelta = new Date();
    timeDelta.setTime(timeDelta.getTime() + ((timezoneHourDiff - deltaHours) * 60 * 60 * 1000));
    return (new Date(timestamp) > timeDelta);
}

function latestFirst(a, b) {
    return (a["RowKey"] > b["RowKey"] ? -1 : 1);
}

module.exports = async function (context, timer) {

    const latestRow = context.bindings.moistureValues && context.bindings.moistureValues.sort(latestFirst)[0] || null;
    const timestamp = latestRow && latestRow.RowKey || null;

    const timezoneDiff = 3; // Finland in the summer: UTC+3
    const timestampDeltaHoursLimit = 12;

    if (isNewerThanDeltaLocalizedTimestamp(timestamp, timestampDeltaHoursLimit, timezoneDiff)) {
        context.log("Logged timestamp value is new enough: " + timestamp);
        context.done();
        return;
    }
    context.log("Logged timestamp value is too old: " + timestamp);

    const alert_data = JSON.stringify([{
        "id": Math.floor(Math.random() * 100) + 1,
        "eventType": "SoilMoistureAlertEvent",
        "subject": "Kasvista ei kuulu mitään", // The plant is not reporting.
        "eventTime": new Date().toISOString(),
        "data": {
            "message": "Edellisestä huonekasvin mullan kapasitanssimittauksesta on yli 24 tuntia: " + timestamp
        }
    }]);

    const req_opts = {
        host: process.env["EventGridTopicHost"],
        path: "/api/events",
        method: "POST",
        headers: {
            "Content-Type": "application/json",
            "aeg-sas-key": process.env["EventGridTopicKey"]
        }
    };

    const alert_req = https.request(req_opts, (alert_res) => {});
    alert_req.on("error", (alert_error) => {
        logger(alert_error);
        reject();
    });

    alert_req.write(alert_data);
    await alert_req.end();

}
