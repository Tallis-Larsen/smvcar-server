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
    // 1. Enable Qt Networking Logs
    QLoggingCategory::setFilterRules(
        "qt.httpserver*=true\n"
        "qt.network*=true"
    );

    qInstallMessageHandler(customMessageHandler);
    QCoreApplication app(argc, argv);

    // Force output flush
    setbuf(stdout, NULL);

    QHttpServer httpServer;

    log("--- SERVER STARTING ---");

    // 2. DEBUG: Print Environment Variables
    // This tells us if DigitalOcean is forcing a specific port
    log("Checking Environment Variables...");
    if (qEnvironmentVariableIsSet("PORT")) {
        log("ENV VAR 'PORT' FOUND: " + qEnvironmentVariable("PORT"));
    } else {
        log("ENV VAR 'PORT' NOT SET. Defaulting to 8080.");
    }

    // 3. Heartbeat Timer
    // This prints a log every 10 seconds.
    // IF YOU SEE THIS IN THE LOGS, the app is alive.
    // IF YOU DO NOT, the app has frozen/crashed silently.
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [](){
        std::cout << "[Heartbeat] App is alive and running..." << std::endl;
    });
    timer.start(10000); // 10 seconds

    // Check file access
    if (QFile::exists("index.html")) {
        log("SUCCESS: index.html found at " + QFileInfo("index.html").absoluteFilePath());
    } else {
        log("CRITICAL: index.html NOT found!");
    }

    QJsonArray ramDatabase = {"Qt6", "Server", "Is", "Online"};

    // Routes
    httpServer.route("/", []() {
        log("Request received for: /");
        return QHttpServerResponse::fromFile("index.html");
    });

    httpServer.route("/api/data", [&ramDatabase]() {
        log("Request received for: /api/data");
        return QHttpServerResponse(QJsonDocument(ramDatabase).toJson(),
                                   QHttpServerResponse::StatusCode::Ok);
    });

    httpServer.setMissingHandler([](const QHttpServerRequest &request, QHttpServerResponder &&responder) {
        log("WARNING: 404 Handler triggered for path: " + request.url().path());
        responder.write(QHttpServerResponse::StatusCode::NotFound);
    });

    // 4. THE LISTEN FIX
    // Use the environment variable if present, otherwise 8080.
    // Bind explicitly to "0.0.0.0" to ensure we accept traffic from outside the container.
    const int portToUse = qEnvironmentVariable("PORT", "8080").toInt();
    const auto port = httpServer.listen(QHostAddress("0.0.0.0"), portToUse);

    if (!port) {
        log("CRITICAL: Failed to listen on port " + QString::number(portToUse));
        return -1;
    }

    log("Server listening on 0.0.0.0:" + QString::number(port));

    return app.exec();
}