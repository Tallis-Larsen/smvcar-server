#include <QCoreApplication>
#include <QHttpServer>
#include <QHttpServerResponse>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QHttpServer httpServer;

    // --- 1. The RAM Database ---
    // A simple array stored in memory. In a real app, you might use a QList<Struct>.
    QJsonArray ramDatabase = {"Qt6", "is", "running", "on", "DigitalOcean"};

    // --- 2. Route: Serve the HTML Website ---
    // When the user visits "/" (the root), send them the index.html file.
    httpServer.route("/", []() {
        return QHttpServerResponse::fromFile("index.html");
    });

    // --- 3. Route: The API Endpoint ---
    // A separate route for your C++ client to download the raw data.
    httpServer.route("/api/data", [&ramDatabase]() {
        return QHttpServerResponse(QJsonDocument(ramDatabase).toJson(),
                                   QHttpServerResponse::StatusCode::Ok);
    });

    // --- 4. Start Listening ---
    // App Platform expects us to listen on port 8080 (defined in app.yaml)
    const auto port = httpServer.listen(QHostAddress::Any, 8080);
    if (!port) {
        qCritical() << "Server failed to listen on port 8080.";
        return -1;
    }

    qDebug() << "Server running on port" << port;
    return app.exec();
}