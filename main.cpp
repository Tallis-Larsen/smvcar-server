#include <QCoreApplication>
#include <QHttpServer>
#include <QHttpServerResponse>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Force logs to flush immediately so they appear in DigitalOcean console
    setbuf(stdout, NULL);

    QHttpServer httpServer;

    // --- Debug: Print current environment ---
    qDebug() << "Starting server...";
    qDebug() << "Current Working Directory:" << QDir::currentPath();

    if (QFile::exists("index.html")) {
        qDebug() << "SUCCESS: index.html found at:" << QFileInfo("index.html").absoluteFilePath();
    } else {
        qCritical() << "ERROR: index.html NOT found in current directory!";
    }

    // --- 1. The RAM Database ---
    QJsonArray ramDatabase = {"Qt6", "is", "running", "on", "DigitalOcean", "Fixed!"};

    // --- 2. Route: Serve the HTML Website ---
    httpServer.route("/", []() {
        qDebug() << "Request received for root '/'";
        return QHttpServerResponse::fromFile("index.html");
    });

    // --- 3. Route: The API Endpoint ---
    httpServer.route("/api/data", [&ramDatabase]() {
        qDebug() << "Request received for API data";
        return QHttpServerResponse(QJsonDocument(ramDatabase).toJson(),
                                   QHttpServerResponse::StatusCode::Ok);
    });

    // --- 4. Start Listening ---
    // Use QHostAddress::AnyIPv4 to ensure we bind to 0.0.0.0 (safest for Docker)
    const auto port = httpServer.listen(QHostAddress::AnyIPv4, 8080);
    if (!port) {
        qCritical() << "Server failed to listen on port 8080.";
        return -1;
    }

    qDebug() << "Server running on port" << port;
    return app.exec();
}