#pragma once
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QSet>
#include <QList>
#include <QFile>

// Single TCP server that routes connections to HTTP or WebSocket handling.
// NOTE: This part written by AI (It's a lotta boilerplate and I don't wanna do it).
class MuxServer : public QTcpServer {
    Q_OBJECT
public:
    explicit MuxServer(QWebSocketServer* webSocketServer, QObject* parent = nullptr);
protected:
    void incomingConnection(qintptr socketDescriptor) override;
private slots:
    void onReadyRead();
private:
    QWebSocketServer* webSocketServer;
    void serveHttp(QTcpSocket* socket);
};

// End AI-written portion

class Server : public QObject {
    Q_OBJECT
public:
    Server(QObject* parent = nullptr);
private:
    QWebSocketServer webSocketServer;
    MuxServer tcpServer;
    QList<QWebSocket*> clients;
    QList<QJsonObject> events;
    QString targetTimeMessage;
    QString targetLapsMessage;
    QSet<QString> invalidCommands;

    void sendMessage(const QString& message);
    void sendRejectMessage(QWebSocket* client, const QString& messageId);
    void invalidateCommand(const QString& commandId);
    void sendBacklog(QWebSocket* client);
private slots:
    void newConnection();
    void processMessage(const QString& message);
    void clientDisconnected();
};