#include "../include/Server.h"

// NOTE: This is my first time writing complex networking logic, so this whole thing is a bit of a mess.

Server::Server(QObject *parent) : QObject(parent), webSocketServer("smvcar-server", QWebSocketServer::NonSecureMode, this) {
    webSocketServer.listen(QHostAddress::Any, 8080);
    connect(&webSocketServer, &QWebSocketServer::newConnection, this, &Server::newConnection);
}

void Server::newConnection() {
    QWebSocket* client = webSocketServer.nextPendingConnection();
    clients.append(client);
    connect(client, &QWebSocket::textMessageReceived, this, &Server::processMessage);
    connect(client, &QWebSocket::disconnected, this, &Server::clientDisconnected);
    sendBacklog(client);
}

void Server::processMessage(const QString& message) {
    // sender() retrieves the object that sent the signal
    QWebSocket* client = qobject_cast<QWebSocket*>(sender());
    QJsonDocument document = QJsonDocument::fromJson(message.toUtf8());

    if (document.isNull() || !document.isObject()) {
        qWarning() << "Received invalid JSON";
        return;
    }

    QJsonObject command = document.object();
    QString function = command["function"].toString();

    if (invalidCommands.contains(command["command_id"].toString())) { return; }

    invalidCommands.insert(command["command_id"].toString());

    // If it's any of these commands, just confirm the message automatically.
    if (function == "startStopwatch") {
        events.append(command);
        sendMessage(message);
        return;
    }
    if (function == "stopStopwatch") {
        events.append(command);
        sendMessage(message);
        return;
    }
    if (function == "resetStopwatch") {
        events.clear();
        sendMessage(message);
        return;
    }
    if (function == "setTargetLaps") {
        targetLapsMessage = message;
        sendMessage(message);
        return;
    }
    if (function == "setTargetTime") {
        targetTimeMessage = message;
        sendMessage(message);
        return;
    }

    events.append(command);

    // Sort the list
    std::sort(events.begin(), events.end(), [](const QJsonObject& a, const QJsonObject& b) {
        return QDateTime::fromString(a["timestamp"].toString(), Qt::ISODateWithMs) < QDateTime::fromString(b["timestamp"].toString(), Qt::ISODateWithMs);
    });

    // Check for any commands within 15 seconds of each other
    for (int i = 1; i < events.size(); i++) {
        QDateTime previousTimestamp = QDateTime::fromString(events[i - 1]["timestamp"].toString(), Qt::ISODateWithMs);
        QDateTime currentTimestamp = QDateTime::fromString(events[i]["timestamp"].toString(), Qt::ISODateWithMs);
        if (previousTimestamp.msecsTo(currentTimestamp) < 15000) {
            QString commandId = events[i]["command_id"].toString();
            // If the invalid command is the one we just received, then just reject it and continue.
            if (commandId == command["command_id"].toString()) {
                sendRejectMessage(client, commandId);
            } else { // If the invalid command is one that was previously validated, we need to remove it from all clients.
                invalidateCommand(commandId);
                // Also sends the received message, because now we know it's not invalid.
                sendMessage(message);
            }
            events.removeAt(i);
            return;
        }
    }

    // If we get this far, then the message is valid and we send it to all clients.
    sendMessage(message);
}

void Server::sendMessage(const QString &message) {
    for (QWebSocket* client : clients) {
        client->sendTextMessage(message);
    }
}

void Server::sendRejectMessage(QWebSocket *client, QString messageId) {
    QJsonObject message;
    message["function"] = "reject";
    message["command_id"] = messageId;
    QString messageString = QString(QJsonDocument(message).toJson(QJsonDocument::Compact));
    client->sendTextMessage(messageString);
}

void Server::invalidateCommand(QString commandId) {
    for (QWebSocket* client : clients) {
        sendRejectMessage(client, commandId);
    }
}

void Server::sendBacklog(QWebSocket* client) {
    // Each client gets a unique message prefix to prevent reconnection id issues
    static char nextMessagePrefix = 'a';
    if (nextMessagePrefix == 'z') {
        nextMessagePrefix = 'a';
    } else {
        nextMessagePrefix++;
    }

    // Separate scopes so I can use the same name multiple times
    {
        QJsonObject message;
        message["function"] = "setPrefix";
        message["message_prefix"] = QString(nextMessagePrefix);
        QString messageString = QString(QJsonDocument(message).toJson(QJsonDocument::Compact));
        client->sendTextMessage(messageString);
    }
    // {
    //     QJsonObject message;
    //     message["function"] = "setId";
    //     message["client_id"] = clientId;
    //     QString messageString = QString(QJsonDocument(message).toJson(QJsonDocument::Compact));
    //     client->sendTextMessage(messageString);
    // }
    if (!targetLapsMessage.isEmpty()) {
        client->sendTextMessage(targetLapsMessage);
    }
    if (!targetTimeMessage.isEmpty()) {
        client->sendTextMessage(targetTimeMessage);
    }
    for (QJsonObject& event : events) {
        QString messageString = QString(QJsonDocument(event).toJson(QJsonDocument::Compact));
        client->sendTextMessage(messageString);
    }

}

void Server::clientDisconnected() {
    // sender() retrieves the object that sent the signal
    QWebSocket* client = qobject_cast<QWebSocket*>(sender());
    if (client) {
        clients.removeAll(client);
        client->deleteLater();
    }
}
