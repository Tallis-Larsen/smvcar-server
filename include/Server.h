#pragma once
#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QSet>

class Server : public QObject {
    Q_OBJECT
public:
    Server(QObject* parent = nullptr);
private:
    QWebSocketServer webSocketServer;
    QList<QWebSocket*> clients;
    QList<QJsonObject> events;
    QString targetTimeMessage;
    QString targetLapsMessage;
    QSet<QString> invalidCommands;

    void sendMessage(const QString& message);
    void sendRejectMessage(QWebSocket *client, QString messageId);
    void invalidateCommand(QString commandId);
    void sendBacklog(QWebSocket* client);
private slots:
    void newConnection();
    void processMessage(const QString& message);
    void clientDisconnected();
};