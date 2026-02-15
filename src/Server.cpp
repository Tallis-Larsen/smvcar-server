#include "../include/Server.h"

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
    QTcpSocket* socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);
    connect(socket, &QTcpSocket::readyRead, this, &MuxServer::onReadyRead);
}

void MuxServer::onReadyRead() {
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) { return; }

    const QByteArray data = socket->peek(1024);

    // Decides if connection is just asking for the HTML page or if it should be converted to a websocket
    if (data.contains("Upgrade: websocket") || data.contains("upgrade: websocket")) {
        disconnect(socket, &QTcpSocket::readyRead, this, &MuxServer::onReadyRead);
        webSocketServer->handleConnection(socket);
    } else {
        disconnect(socket, &QTcpSocket::readyRead, this, &MuxServer::onReadyRead);
        serveHttp(socket);
    }
}

void MuxServer::serveHttp(QTcpSocket *socket) {
    const QByteArray request = socket->readAll();

    QFile file("index.html");
    QByteArray body;

    if (file.open(QIODevice::ReadOnly)) {
        body = file.readAll();
    } else {
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
        return;
    }
}

void Server::newConnection() {
    QWebSocket* client = webSocketServer.nextPendingConnection();
    client->setParent(this);
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
        return;
    }

    QJsonObject command = document.object();
    QString function = command[FUNCTION].toString();

    if (invalidCommands.contains(command[COMMAND_ID].toString())) { return; }

    invalidCommands.insert(command[COMMAND_ID].toString());

    // If it's any of these commands, just confirm the message automatically.
    if (function == START_STOPWATCH) {
        events.append(command);
        sendMessage(message);
        return;
    }
    if (function == STOP_STOPWATCH) {
        events.append(command);
        sendMessage(message);
        return;
    }
    if (function == RESET_STOPWATCH) {
        events.clear();
        sendMessage(message);
        return;
    }
    if (function == SET_TARGET_LAPS) {
        targetLapsMessage = message;
        sendMessage(message);
        return;
    }
    if (function == SET_TARGET_TIME) {
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
            break;
        }
    }

    // If stopwatch is not running, we reject the lap command.
    if (!isRunning) {
        sendRejectMessage(client, command[COMMAND_ID].toString());
        return;
    }

    events.append(command);

    // Sort the list
    std::sort(events.begin(), events.end(), [](const QJsonObject& a, const QJsonObject& b) {
        return QDateTime::fromString(a[TIMESTAMP].toString(), Qt::ISODateWithMs) < QDateTime::fromString(b[TIMESTAMP].toString(), Qt::ISODateWithMs);
    });

    // Check for any commands within 15 seconds of each other
    for (int i = 1; i < events.size(); i++) {
        QDateTime previousTimestamp = QDateTime::fromString(events[i - 1][TIMESTAMP].toString(), Qt::ISODateWithMs);
        QDateTime currentTimestamp = QDateTime::fromString(events[i][TIMESTAMP].toString(), Qt::ISODateWithMs);
        if (previousTimestamp.msecsTo(currentTimestamp) < 5000) {
            QString commandId = events[i][COMMAND_ID].toString();
            // If the invalid command is the one we just received, then just reject it and continue.
            if (commandId == command[COMMAND_ID].toString()) {
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

void Server::sendRejectMessage(QWebSocket* client, const QString& messageId) {
    QJsonObject message;
    message[FUNCTION] = REJECT;
    message[COMMAND_ID] = messageId;
    QString messageString = QString::fromUtf8(QJsonDocument(message).toJson(QJsonDocument::Compact));
    client->sendTextMessage(messageString);
}

void Server::invalidateCommand(const QString& commandId) {
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
        clients.removeAll(client);
        client->deleteLater();
    }
}
