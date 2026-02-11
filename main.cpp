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
#include <QLoggingCategory>

// 1. Custom Message Handler
// This ensures we catch every single whisper from the Qt engine.
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    std::cout << "[Qt] " << qPrintable(msg) << std::endl;
}

void log(const QString &msg) {
    std::cout << "[App] " << qPrintable(msg) << std::endl;
}

int main(int argc, char *argv[])
{
    // 2. Enable DEEP Internal Logging
    // This is the most important change. It tells Qt to log low-level
    // network details. If a connection hits the server, this WILL see it.
    QLoggingCategory::setFilterRules(
        "qt.httpserver*=true\n"
        "qt.network*=true"
    );

    qInstallMessageHandler(customMessageHandler);
    QCoreApplication app(argc, argv);

    // Force immediate log flushing
    setbuf(stdout, NULL);

    QHttpServer httpServer;

    log("--- SERVER STARTING ---");
    log("Listening on explicit port: 8080");

    // Check for index.html existence
    if (QFile::exists("index.html")) {
        log("SUCCESS: index.html found.");
    } else {
        log("CRITICAL ERROR: index.html NOT found!");
    }

    QJsonArray ramDatabase = {"Qt6", "is", "working", "on", "App Platform"};

    // 3. Main Route
    httpServer.route("/", []() {
        log("Request received for route: /");
        return QHttpServerResponse::fromFile("index.html");
    });

    // 4. API Route
    httpServer.route("/api/data", [&ramDatabase]() {
        log("Request received for route: /api/data");
        return QHttpServerResponse(QJsonDocument(ramDatabase).toJson(),
                                   QHttpServerResponse::StatusCode::Ok);
    });

    // 5. Catch-All Handler
    // If the browser requests /favicon.ico or anything else, this logs it.
    httpServer.setMissingHandler([](const QHttpServerRequest &request, QHttpServerResponder &&responder) {
        log("WARNING: 404 Handler triggered for path: " + request.url().path());
        responder.write(QHttpServerResponse::StatusCode::NotFound);
    });

    // 6. Start Listening
    // We stick to AnyIPv4 to ensure 0.0.0.0 binding (standard for Docker)
    const auto port = httpServer.listen(QHostAddress::AnyIPv4, 8080);

    if (!port) {
        log("CRITICAL: Failed to listen on port 8080");
        return -1;
    }

    log("Server listening on port " + QString::number(port));

    return app.exec();
}