#include "../include/Server.h"
#include <QDebug>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>

const QString COMMAND_ID = "command_id";
const QString FUNCTION = "function";
const QString TIMESTAMP = "timestamp";
const QString REJECT = "reject";
const QString START_STOPWATCH = "startStopwatch";
const QString STOP_STOPWATCH = "stopStopwatch";
const QString RESET_STOPWATCH = "resetStopwatch";
const QString SET_PREFIX = "setPrefix";
const QString SET_TARGET_LAPS = "setTargetLaps";
const QString SET_TARGET_TIME = "setTargetTime";
const QString MESSAGE_PREFIX = "message_prefix";

// NOTE: This is my first time writing complex networking logic, so this whole thing is a bit of a mess.

MuxServer::MuxServer(QWebSocketServer* webSocketServer, QObject* parent)
    : QTcpServer(parent), webSocketServer(webSocketServer) {}

void MuxServer::incomingConnection(qintptr socketDescriptor) {
    qDebug() << "Client connection attempt with socket descriptor:" << socketDescriptor;
    QTcpSocket* socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);
    connect(socket, &QTcpSocket::readyRead, this, &MuxServer::onReadyRead);
}

void MuxServer::onReadyRead() {
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        qWarning() << "onReadyRead triggered without a valid socket.";
        return;
    }

    const QByteArray data = socket->peek(1024);

    // Decides if connection is just asking for the HTML page or if it should be converted to a websocket
    if (data.contains("Upgrade: websocket") || data.contains("upgrade: websocket")) {
        qDebug() << "Attempting to upgrade connection to WebSocket for" << socket->peerAddress().toString();
        disconnect(socket, &QTcpSocket::readyRead, this, &MuxServer::onReadyRead);
        webSocketServer->handleConnection(socket);
    } else {
        disconnect(socket, &QTcpSocket::readyRead, this, &MuxServer::onReadyRead);
        serveHttp(socket);
    }
}

void MuxServer::serveHttp(QTcpSocket *socket) {
    qDebug() << "Serving HTTP request to:" << socket->peerAddress().toString();
    const QByteArray request = socket->readAll();

    QFile file("index.html");
    QByteArray body;

    if (file.open(QIODevice::ReadOnly)) {
        body = file.readAll();
    } else {
        qWarning() << "Could not open index.html";
        body = "<h1>404 Not Found</h1>";
    }

    const QByteArray response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: " + QByteArray::number(body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" + body;

    socket->write(response);
    connect(socket, &QTcpSocket::bytesWritten, socket, [socket]() {
        if (socket->bytesToWrite() == 0) { socket->disconnectFromHost(); }});

}

Server::Server(QObject *parent) : QObject(parent), webSocketServer("smvcar-server", QWebSocketServer::NonSecureMode, this),
    tcpServer(&webSocketServer, this) {

    const quint16 port = static_cast<quint16>(qEnvironmentVariable("PORT", "8080").toInt());
    connect(&webSocketServer, &QWebSocketServer::newConnection, this, &Server::newConnection);

    if (!tcpServer.listen(QHostAddress::Any, port)) {
        qCritical() << "Server failed to listen on port" << port;
        return;
    }
    qInfo() << "Server listening on port" << port;
}

void Server::newConnection() {
    QWebSocket* client = webSocketServer.nextPendingConnection();
    qInfo() << "Client connected:" << client->peerAddress().toString() << client->peerPort();
    client->setParent(this);
    clients.append(client);
    connect(client, &QWebSocket::textMessageReceived, this, &Server::processMessage);
    connect(client, &QWebSocket::disconnected, this, &Server::clientDisconnected);
    sendBacklog(client);
}

void Server::processMessage(const QString& message) {
    // sender() retrieves the object that sent the signal
    QWebSocket* client = qobject_cast<QWebSocket*>(sender());
    qInfo() << "Received message from" << client->peerAddress().toString() << ":" << message;

    QJsonDocument document = QJsonDocument::fromJson(message.toUtf8());

    if (document.isNull() || !document.isObject()) {
        qWarning() << "Received invalid JSON from" << client->peerAddress().toString() << ":" << message;
        return;
    }

    QJsonObject command = document.object();
    QString function = command[FUNCTION].toString();
    QString commandId = command[COMMAND_ID].toString();

    qInfo() << "Processing command" << commandId << "with function" << function;

    // Reject commands that have been seen before
    if (invalidCommands.contains(commandId)) {
        qDebug() << "Ignoring duplicate command" << commandId;
        return;
    }

    invalidCommands.insert(commandId);

    // If it's any of these commands, just confirm the message automatically.
    if (function == START_STOPWATCH) {
        qInfo() << "Starting stopwatch. Storing event and broadcasting.";
        events.append(command);
        sendMessage(message);
        return;
    }
    if (function == STOP_STOPWATCH) {
        qInfo() << "Stopping stopwatch. Storing event and broadcasting.";
        events.append(command);
        sendMessage(message);
        return;
    }
    if (function == RESET_STOPWATCH) {
        qInfo() << "Resetting stopwatch. Clearing events and broadcasting.";
        events.clear();
        sendMessage(message);
        return;
    }
    if (function == SET_TARGET_LAPS) {
        qInfo() << "Setting target laps and broadcasting.";
        targetLapsMessage = message;
        sendMessage(message);
        return;
    }
    if (function == SET_TARGET_TIME) {
        qInfo() << "Setting target time and broadcasting.";
        targetTimeMessage = message;
        sendMessage(message);
        return;
    }

    // Check if stopwatch is running
    bool isRunning = false;
    for (QJsonObject& event : events) {
        if (event[FUNCTION].toString() == START_STOPWATCH) {
            isRunning = true;
        } else if (event[FUNCTION].toString() == STOP_STOPWATCH) {
            isRunning = false;
        }
    }
    qDebug() << "Is stopwatch running?" << isRunning;

    // If stopwatch is not running, we reject the lap command.
    if (!isRunning) {
        qDebug() << "Rejecting lap command" << commandId << "because stopwatch is not running.";
        sendRejectMessage(client, commandId);
        return;
    }

    qInfo() << "Lap command" << commandId << "is valid for now. Appending to events.";
    events.append(command);

    // Sort the list
    qDebug() << "Sorting events list.";
    std::sort(events.begin(), events.end(), [](const QJsonObject& a, const QJsonObject& b) {
        return QDateTime::fromString(a[TIMESTAMP].toString(), Qt::ISODateWithMs) < QDateTime::fromString(b[TIMESTAMP].toString(), Qt::ISODateWithMs);
    });

    // Check for any commands within 5 seconds of each other
    for (int i = 1; i < events.size(); i++) {
        QDateTime previousTimestamp = QDateTime::fromString(events[i - 1][TIMESTAMP].toString(), Qt::ISODateWithMs);
        QDateTime currentTimestamp = QDateTime::fromString(events[i][TIMESTAMP].toString(), Qt::ISODateWithMs);
        if (previousTimestamp.msecsTo(currentTimestamp) < 5000) {
            QString conflictingCommandId = events[i][COMMAND_ID].toString();
            qDebug() << "Found command" << conflictingCommandId << "too close to previous command. Invalidating.";
            // If the invalid command is the one we just received, then just reject it and continue.
            if (conflictingCommandId == command[COMMAND_ID].toString()) {
                sendRejectMessage(client, conflictingCommandId);
            } else { // If the invalid command is one that was previously validated, we need to remove it from all clients.
                invalidateCommand(conflictingCommandId);
                // Also sends the received message, because now we know it's not invalid.
                sendMessage(message);
            }
            events.removeAt(i);
            return;
        }
    }

    // If we get this far, then the message is valid and we send it to all clients.
    qInfo() << "Command" << commandId << "is valid. Broadcasting to all clients.";
    sendMessage(message);
}

void Server::sendMessage(const QString &message) {
    qDebug() << "Broadcasting message to" << clients.size() << "clients:" << message;
    for (QWebSocket* client : clients) {
        client->sendTextMessage(message);
    }
}

void Server::sendRejectMessage(QWebSocket* client, const QString& messageId) {
    QJsonObject message;
    message[FUNCTION] = REJECT;
    message[COMMAND_ID] = messageId;
    QString messageString = QString::fromUtf8(QJsonDocument(message).toJson(QJsonDocument::Compact));
    qDebug() << "Sending reject for command" << messageId << "to" << client->peerAddress().toString();
    client->sendTextMessage(messageString);
}

void Server::invalidateCommand(const QString& commandId) {
    qDebug() << "Invalidating command" << commandId << "for all clients.";
    for (QWebSocket* client : clients) {
        sendRejectMessage(client, commandId);
    }
}

void Server::sendBacklog(QWebSocket* client) {
    // Each client gets a unique message prefix to prevent reconnection id issues
    static char nextMessagePrefix = 'a';
    if (nextMessagePrefix == 'z') {
        nextMessagePrefix = 'a';
        invalidCommands.clear(); // Dangerous? yes. Lazy? absolutely. Functional? maybe.
    } else {
        nextMessagePrefix++;
    }

    // Separate scopes so I can use the same name multiple times
    {
        QJsonObject message;
        message[FUNCTION] = SET_PREFIX;
        message[MESSAGE_PREFIX] = QString(nextMessagePrefix);
        QString messageString = QString::fromUtf8(QJsonDocument(message).toJson(QJsonDocument::Compact));
        client->sendTextMessage(messageString);
    }
    if (!targetLapsMessage.isEmpty()) {
        client->sendTextMessage(targetLapsMessage);
    }
    if (!targetTimeMessage.isEmpty()) {
        client->sendTextMessage(targetTimeMessage);
    }
    for (QJsonObject& event : events) {
        QString messageString = QString::fromUtf8(QJsonDocument(event).toJson(QJsonDocument::Compact));
        client->sendTextMessage(messageString);
    }

}

void Server::clientDisconnected() {
    // sender() retrieves the object that sent the signal
    QWebSocket* client = qobject_cast<QWebSocket*>(sender());
    if (client) {
        qInfo() << "Client disconnected:" << client->peerAddress().toString() << client->peerPort();
        clients.removeAll(client);
        client->deleteLater();
    }
}
