module.exports = async function (context, req) {
    if (req.query.value || (req.body && req.body.value)) {
        context.bindings.moistureValues = [];

        // Finland: UTC+2 (+3 during summer)
        const timestamp = req.query.timestamp ||
            (req.body && req.body.timestamp) ||
            getLocalizedTimeString(2);

        context.bindings.moistureValues.push({
            PartitionKey: "LogValue",
            RowKey: timestamp,
            Value: req.query.value || req.body.value
        });

        context.res = {
            body: "Logged value " + (req.query.value || req.body.value)
        };

        await alertIfDry(req.query.value || req.body.value, context.log);
    }
    else {
        context.res = {
            status: 400,
            body: "Please pass a value on the query string or in the request body."
        };
    }
    context.done();
};

var https = require("https");

function getLocalizedTimeString(timezoneHourDiff) {
    const today = new Date();
    today.setTime(today.getTime() + (timezoneHourDiff * 60 * 60 * 1000));
    const date = today.toISOString().substring(0, 10);
    const time = today.toISOString().substring(11, 19);
    return date + " " + time;
}

function alertIfDry(value, logger) {
    return new Promise((resolve, reject) => {

        // This determines whether to send email or not.
        // Usually the upper limit is around 480-600 depending on soil, battery level and possibly even sensor wear.
        // For new soil, which isn't that tightly packed, 480-500 is a good value.
        // For a soil that's been used for a few months, 580-600 is good.
        // When batteries start to run out, the values go down, eventually staying below 450.
        // The rate at which the capacitance value should grow is usually around 12-25 every 24 hours with non-extreme moisture values.
        const capacitance_limit = process.env["CapacitanceLimit"] || 600

        if (value < capacitance_limit) {
            return resolve();
        }
        logger("Value is too high, sending alert.");

        const alert_data = JSON.stringify([{
            "id": Math.floor(Math.random() * 100) + 1,
            "eventType": "SoilMoistureAlertEvent",
            "subject": "Kasvi tarvitsee vettä", // The plant needs water.
            "eventTime": new Date().toISOString(),
            "data": {
                "message": "Huonekasvi tarvitsee vettä.\nMitattu kapasitanssiarvo: " + value.toString()
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
        alert_req.end(resolve);
        logger("Finished alert request.");
    });
}
