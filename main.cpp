#include <QCoreApplication>
#include <QHttpServer>
#include <QHttpServerResponse>
#include <QHttpServerResponder>
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

    // Check for the file immediately on startup
    if (QFile::exists("index.html")) {
        log("SUCCESS: index.html found at: " + QFileInfo("index.html").absoluteFilePath());
    } else {
        log("CRITICAL ERROR: index.html NOT found! Listing directory...");
        QDir dir(".");
        for (const QString &entry : dir.entryList()) {
            log(" - " + entry);
        }
    }

    // 1. RAM Database
    QJsonArray ramDatabase = {"Qt6", "is", "working", "perfectly", "on", "App Platform"};

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

    // 4. FIX: Corrected signature for setMissingHandler
    // This now accepts the 'responder' object which we use to send the error.
    httpServer.setMissingHandler([](const QHttpServerRequest &request, QHttpServerResponder &&responder) {
        log("WARNING: 404 Missing Handler triggered for path: " + request.url().path());
        responder.sendStatusCode(QHttpServerResponse::StatusCode::NotFound);
    });

    // 5. Start Listening
    // We use QHostAddress::Any to ensure we listen on all interfaces (IPv4/IPv6)
    const auto port = httpServer.listen(QHostAddress::Any, 8080);

    if (!port) {
        log("CRITICAL: Failed to listen on port 8080");
        return -1;
    }

    log("Server running on port " + QString::number(port));

    return app.exec();
}