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

// --- 1. Robust Logging Helper ---
// This ensures every single log (qDebug, qWarning, std::cout) gets flushed
// immediately so you see it in the DigitalOcean console.
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QByteArray localMsg = msg.toLocal8Bit();
    std::cout << "[Qt] " << localMsg.constData() << std::endl;
}

void log(const QString &msg) {
    std::cout << "[App] " << qPrintable(msg) << std::endl;
}

int main(int argc, char *argv[])
{
    // Install the custom logger to catch internal Qt warnings
    qInstallMessageHandler(customMessageHandler);

    QCoreApplication app(argc, argv);
    QHttpServer httpServer;

    log("--- SERVER STARTING (IPv4 Mode) ---");
    log("Current Working Directory: " + QDir::currentPath());

    if (QFile::exists("index.html")) {
        log("SUCCESS: index.html found.");
    } else {
        log("CRITICAL: index.html NOT found!");
    }

    QJsonArray ramDatabase = {"Qt6", "is", "working", "on", "IPv4"};

    // --- 2. Root Route ---
    httpServer.route("/", []() {
        log("Request received for route: /");
        return QHttpServerResponse::fromFile("index.html");
    });

    // --- 3. API Route ---
    httpServer.route("/api/data", [&ramDatabase]() {
        log("Request received for route: /api/data");
        return QHttpServerResponse(QJsonDocument(ramDatabase).toJson(),
                                   QHttpServerResponse::StatusCode::Ok);
    });

    // --- 4. Catch-All Handler (Debug 404s) ---
    httpServer.setMissingHandler([](const QHttpServerRequest &request, QHttpServerResponder &&responder) {
        log("WARNING: 404 Handler triggered for path: " + request.url().path());
        responder.write(QHttpServerResponse::StatusCode::NotFound);
    });

    // --- 5. THE FIX: Force IPv4 Binding ---
    // QHostAddress::Any might bind to IPv6 (::), which can cause
    // docker port mapping to fail on some cloud providers.
    const auto port = httpServer.listen(QHostAddress::AnyIPv4, 8080);

    if (!port) {
        log("CRITICAL: Failed to listen on port 8080");
        return -1;
    }

    log("Server listening on 0.0.0.0:" + QString::number(port));

    return app.exec();
}