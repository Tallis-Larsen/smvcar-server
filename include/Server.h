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
private slots:
    void newConnection();
    void processMessage(const QString& message);
    void clientDisconnected();
};