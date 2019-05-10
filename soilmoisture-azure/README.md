# soilmoisture

Azure Function App which:
* provides an endpoint for the ESP8266 to report values to (LogValue function)
* allows a user to list measured values (ListLoggedValues function)
* monitors the reporting frequency (MonitorTimer function)
* sends alerts via SendGrid (SendEmailAlert function)

Uses the Event Grid and SendGrid functionality for sending emails, and Table Storage to store the values.

Needs the following application settings:
* AzureWebJobsStorage
* EmailAlertRecipient
* EventGridTopicHost
* EventGridTopicKey
* SendGridAPIKey
