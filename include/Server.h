#pragma once
#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

class Server : public QObject {
    Q_OBJECT
public:
    Server(QObject* parent = nullptr);
private:
    QWebSocketServer webSocketServer;
    QList<QWebSocket*> clients;
    QList<QJsonObject> events;
    QMap<QWebSocket*, int> clientIds;
    QString targetTimeMessage;
    QString targetLapsMessage;

    QWebSocket* getClientById(int clientId);
    void sendMessage(const QString& message);
    void sendRejectMessage(QWebSocket *client, int messageId);
    void invalidateCommand(int commandId);
private slots:
    void newConnection();
    void processMessage(const QString& message);
    void clientDisconnected();
};