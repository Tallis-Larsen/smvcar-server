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
#include <QTimer>

// Custom Message Handler to ensure we see everything
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    std::cout << "[Qt] " << qPrintable(msg) << std::endl;
}

void log(const QString &msg) {
    std::cout << "[App] " << qPrintable(msg) << std::endl;
}

int main(int argc, char *argv[])
{
    // Enable Networking Logs
    QLoggingCategory::setFilterRules("qt.httpserver*=true\nqt.network*=true");
    qInstallMessageHandler(customMessageHandler);

    QCoreApplication app(argc, argv);
    setbuf(stdout, NULL); // Disable buffering to ensure logs appear instantly

    QHttpServer httpServer;

    log("--- SERVER STARTING (API Fix) ---");

    // Check Port Env Var
    if (qEnvironmentVariableIsSet("PORT")) {
        log("ENV VAR 'PORT' FOUND: " + qEnvironmentVariable("PORT"));
    } else {
        log("ENV VAR 'PORT' NOT SET. Using default.");
    }

    // Heartbeat to prove process is running
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [](){
        std::cout << "[Heartbeat] App is alive..." << std::endl;
    });
    timer.start(10000);

    // Pre-load HTML into memory
    QString htmlContent = "<h1>Error: HTML file not found</h1>";
    if (QFile::exists("index.html")) {
        log("SUCCESS: index.html found.");
        QFile file("index.html");
        if (file.open(QIODevice::ReadOnly)) {
            // Read as UTF-8 string, though readAll returns QByteArray naturally
            htmlContent = QString::fromUtf8(file.readAll());
            log("Loaded index.html into memory (" + QString::number(htmlContent.size()) + " bytes)");
        }
    } else {
        log("CRITICAL: index.html NOT found!");
    }

    QJsonArray ramDatabase = {"Qt6", "Server", "Is", "Online"};

    // 1. Root Route
    // Capture htmlContent by copy
    httpServer.route("/", [htmlContent]() {
        log("Request received for: /");

        // FIX: Construct response with data/status first, then set header.
        // QHttpServerResponse constructor expects QByteArray, not QString.
        QHttpServerResponse response(htmlContent.toUtf8(), QHttpServerResponse::StatusCode::Ok);
        response.setHeader("Content-Type", "text/html");
        return response;
    });

    // 2. API Route
    httpServer.route("/api/data", [&ramDatabase]() {
        log("Request received for: /api/data");
        return QHttpServerResponse(QJsonDocument(ramDatabase).toJson(),
                                   QHttpServerResponse::StatusCode::Ok);
    });

    // 3. Fallback / Missing Handler
    // FIX: Use responder.write(data, mimeType, status) overload
    httpServer.setMissingHandler([htmlContent](const QHttpServerRequest &request, QHttpServerResponder &&responder) {
        log("WARNING: Catch-All/Missing Handler triggered for path: " + request.url().path());

        // responder.write does not accept a QHttpServerResponse object.
        // It accepts raw data + mime type + status.
        responder.write(htmlContent.toUtf8(), "text/html", QHttpServerResponse::StatusCode::Ok);
    });

    // 4. Listen
    const int portToUse = qEnvironmentVariable("PORT", "8080").toInt();
    const auto port = httpServer.listen(QHostAddress::Any, portToUse);

    if (!port) {
        log("CRITICAL: Failed to listen on port " + QString::number(portToUse));
        return -1;
    }

    log("Server listening on port " + QString::number(port));

    return app.exec();
}