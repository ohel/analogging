module.exports = function (context, event) {

    if (!(event && event.subject && event.data && event.data.message)) {
        context.done();
        return;
    }

    const recipient = process.env["EmailAlertRecipient"];

    const message = {
        personalizations: [ { to: [ { email: recipient } ] } ],
        subject: event.subject,
        content: [{
            type: 'text/plain',
            value: event.data.message
        }]
    };

    context.log("Sending email to: " + recipient);
    context.done(null, message);
};
