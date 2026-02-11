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

    log("--- SERVER STARTING (Fixed Build) ---");

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

    // Pre-load HTML into memory to guarantee availability
    QString htmlContent = "<h1>Error: HTML file not found</h1>";
    if (QFile::exists("index.html")) {
        log("SUCCESS: index.html found.");
        QFile file("index.html");
        if (file.open(QIODevice::ReadOnly)) {
            htmlContent = file.readAll();
            log("Loaded index.html into memory (" + QString::number(htmlContent.size()) + " bytes)");
        }
    } else {
        log("CRITICAL: index.html NOT found!");
    }

    QJsonArray ramDatabase = {"Qt6", "Server", "Is", "Online"};

    // 1. Root Route
    httpServer.route("/", [htmlContent]() {
        log("Request received for: /");
        return QHttpServerResponse(htmlContent, QHttpServerResponse::StatusCode::Ok, QHttpServerResponse::HeaderList{{"Content-Type", "text/html"}});
    });

    // 2. API Route
    httpServer.route("/api/data", [&ramDatabase]() {
        log("Request received for: /api/data");
        return QHttpServerResponse(QJsonDocument(ramDatabase).toJson(),
                                   QHttpServerResponse::StatusCode::Ok);
    });

    // 3. Fallback / Missing Handler
    // We removed the QRegularExpression route because it caused a build error.
    // Instead, we use this handler to catch EVERYTHING else.
    // We serve the HTML page here too (SPA behavior) to ensure you see SOMETHING even if the path is wrong.
    httpServer.setMissingHandler([htmlContent](const QHttpServerRequest &request, QHttpServerResponder &&responder) {
        log("WARNING: Catch-All/Missing Handler triggered for path: " + request.url().path());

        QHttpServerResponse response(htmlContent, QHttpServerResponse::StatusCode::Ok, QHttpServerResponse::HeaderList{{"Content-Type", "text/html"}});
        responder.write(std::move(response));
    });

    // 4. Listen
    // Using QHostAddress::Any to support both IPv6 (DigitalOcean internal) and IPv4.
    const int portToUse = qEnvironmentVariable("PORT", "8080").toInt();
    const auto port = httpServer.listen(QHostAddress::Any, portToUse);

    if (!port) {
        log("CRITICAL: Failed to listen on port " + QString::number(portToUse));
        return -1;
    }

    log("Server listening on port " + QString::number(port));

    return app.exec();
}