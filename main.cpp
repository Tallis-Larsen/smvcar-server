#include <QCoreApplication>
#include <QHttpServer>
#include <QHttpServerResponse>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <iostream>

// Helper to force logs to flush immediately (DigitalOcean logs can be laggy)
void log(const QString &msg) {
    std::cout << qPrintable(msg) << std::endl;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QHttpServer httpServer;

    log("--- SERVER STARTING ---");
    log("Current Working Directory: " + QDir::currentPath());

    if (QFile::exists("index.html")) {
        log("SUCCESS: index.html found.");
    } else {
        log("CRITICAL ERROR: index.html NOT found!");
    }

    // 1. RAM Database
    QJsonArray ramDatabase = {"Qt6", "is", "working", "perfectly"};

    // 2. Main Route
    httpServer.route("/", []() {
        log("Request received for route: /");
        return QHttpServerResponse::fromFile("index.html");
    });

    // 3. API Route
    httpServer.route("/api/data", [&ramDatabase]() {
        log("Request received for route: /api/data");
        return QHttpServerResponse(QJsonDocument(ramDatabase).toJson(),
                                   QHttpServerResponse::StatusCode::Ok);
    });

    // 4. FIX: Catch-all handler
    // If the request hits the server but doesn't match "/" or "/api/data",
    // this will print exactly what path the browser is asking for.
    httpServer.setMissingHandler([](const QHttpServerRequest &request) {
        log("WARNING: 404 Missing Handler triggered for path: " + request.url().path());
        return QHttpServerResponse::StatusCode::NotFound;
    });

    // 5. Start Listening
    // Switched back to QHostAddress::Any to support both IPv4 and IPv6
    const auto port = httpServer.listen(QHostAddress::Any, 8080);

    if (!port) {
        log("CRITICAL: Failed to listen on port 8080");
        return -1;
    }

    log("Server running on port " + QString::number(port));

    return app.exec();
}