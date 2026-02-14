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
}

void Server::processMessage(const QString& message) {
    QJsonDocument document = QJsonDocument::fromJson(message.toUtf8());

    if (document.isNull() || !document.isObject()) {
        qWarning() << "Received invalid JSON";
        return;
    }

    QJsonObject command = document.object();

    QString function = command["function"].toString();

    // If it's any of these commands, just confirm the message automatically.
    if (function == "startStopwatch") {
        events.append(message);
        sendMessage(message);
        return;
    }
    if (function == "stopStopwatch") {
        events.append(message);
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


    events.append(message);

    // NOTE: Re-parsing the strings every time we do an operation on them is very inefficient.
    // However, it keeps the events list as strings which makes it much easier to pass messages to the clients.

    // Sort the list
    std::sort(events.begin(), events.end(), [](const QString& a, const QString& b) {
        return QJsonDocument::fromJson(a.toUtf8()).object()["timestamp"].toInteger()
        < QJsonDocument::fromJson(b.toUtf8()).object()["timestamp"].toInteger();
    });

    // Check for any commands within 15 seconds of each other
    for (int i = 1; i < events.size(); i++) {
        qint64 previousTimestamp = QJsonDocument::fromJson(events[i-1].toUtf8()).object()["timestamp"].toInteger();
        qint64 currentTimestamp = QJsonDocument::fromJson(events[i].toUtf8()).object()["timestamp"].toInteger();
        if (currentTimestamp - previousTimestamp < 15000) {
            int commandId = QJsonDocument::fromJson(events[i].toUtf8()).object()["id"].toInt();
            events.removeAt(i);
            sendRejectMessage(commandId);
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

void Server::sendRejectMessage(int messageId) {
    QJsonObject message;
    message["function"] = "reject";
    message["id"] = messageId;
    QString messageString = QString(QJsonDocument(message).toJson(QJsonDocument::Compact));
    // While this does send a reject response to ALL clients,
    // the clients without the corresponding message in queue should ignore it so it should be fine.
    sendMessage(messageString);
}

void Server::clientDisconnected() {
    // sender() retrieves the object that sent the signal
    QWebSocket* client = qobject_cast<QWebSocket*>(sender());
    if (client) {
        clients.removeOne(client);
        client->deleteLater();
    }
}
