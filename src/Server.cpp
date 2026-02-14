#include "../include/Server.h"

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
    // sender() returns the sender of the signal
    QWebSocket* client = qobject_cast<QWebSocket*>(sender());
    QJsonDocument document = QJsonDocument::fromJson(message.toUtf8());
    if (document.isNull() || !document.isObject()) {
        qWarning() << "Received invalid JSON";
        return;
    }
    QJsonObject command = document.object();
    int commandId = command["id"].toInt();
    QString functionName = command["function"].toString();
    QJsonObject functionParameters = command["parameters"].toObject();

    if (functionName == "startStopwatch") {
        QString startTimeString = functionParameters["startTime"].toString();
        QDateTime startTime = QDateTime::fromString(startTimeString, Qt::ISODateWithMs);

    }

}

void Server::clientDisconnected() {

}
