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
#include <QRegularExpression>

// Custom Message Handler
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    std::cout << "[Qt] " << qPrintable(msg) << std::endl;
}

void log(const QString &msg) {
    std::cout << "[App] " << qPrintable(msg) << std::endl;
}

int main(int argc, char *argv[])
{
    QLoggingCategory::setFilterRules("qt.httpserver*=true\nqt.network*=true");
    qInstallMessageHandler(customMessageHandler);
    QCoreApplication app(argc, argv);
    setbuf(stdout, NULL);

    QHttpServer httpServer;

    log("--- SERVER STARTING (Dual Stack Mode) ---");

    // Check Port Env Var
    if (qEnvironmentVariableIsSet("PORT")) {
        log("ENV VAR 'PORT' FOUND: " + qEnvironmentVariable("PORT"));
    } else {
        log("ENV VAR 'PORT' NOT SET. Using default.");
    }

    // Heartbeat
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [](){
        std::cout << "[Heartbeat] App is alive..." << std::endl;
    });
    timer.start(5000); // 5 seconds

    // File Check
    if (QFile::exists("index.html")) {
        log("SUCCESS: index.html found.");
    } else {
        log("CRITICAL: index.html NOT found!");
    }

    QJsonArray ramDatabase = {"Qt6", "Server", "Is", "Online"};

    // 1. Root Route
    httpServer.route("/", []() {
        log("Request received for: /");
        return QHttpServerResponse::fromFile("index.html");
    });

    // 2. API Route
    httpServer.route("/api/data", [&ramDatabase]() {
        log("Request received for: /api/data");
        return QHttpServerResponse(QJsonDocument(ramDatabase).toJson(),
                                   QHttpServerResponse::StatusCode::Ok);
    });

    // 3. NEW: Regex Catch-All
    // This catches ANYTHING that isn't caught above.
    // It's more aggressive than setMissingHandler.
    httpServer.route(QRegularExpression(".*"), [](const QHttpServerRequest &request) {
        log("WARNING: Catch-All Route triggered for path: " + request.url().path());
        return QHttpServerResponse::StatusCode::NotFound;
    });

    // 4. Missing Handler (Backup)
    httpServer.setMissingHandler([](const QHttpServerRequest &request, QHttpServerResponder &&responder) {
        log("WARNING: Missing Handler triggered for path: " + request.url().path());
        responder.write(QHttpServerResponse::StatusCode::NotFound);
    });

    // 5. THE FIX: QHostAddress::Any (IPv4 + IPv6)
    // We bind to both stacks to ensure DO's router can find us.
    const int portToUse = qEnvironmentVariable("PORT", "8080").toInt();
    const auto port = httpServer.listen(QHostAddress::Any, portToUse);

    if (!port) {
        log("CRITICAL: Failed to listen on port " + QString::number(portToUse));
        return -1;
    }

    log("Server listening on port " + QString::number(port));

    return app.exec();
}