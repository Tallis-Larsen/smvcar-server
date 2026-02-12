// Basic test program created by AI

#include <QCoreApplication>
#include <QHttpServer>
#include <QHttpServerResponse>
#include <QFile>
#include <iostream>

void log(const QString &msg) {
    std::cout << "[Server] " << qPrintable(msg) << std::endl;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QHttpServer server;

    // Load index.html into memory
    QFile file("index.html");
    QString html = "<h1>Error: index.html not found</h1>";
    if (file.open(QIODevice::ReadOnly)) {
        html = QString::fromUtf8(file.readAll());
        log("index.html loaded successfully");
    } else {
        log("WARNING: index.html not found");
    }

    // Serve HTML at root
    server.route("/", [html]() {
        QHttpServerResponse response(html.toUtf8(), QHttpServerResponse::StatusCode::Ok);
        response.setHeader("Content-Type", "text/html");
        return response;
    });

    // Start listening
    const int port = qEnvironmentVariable("PORT", "8080").toInt();
    if (!server.listen(QHostAddress::Any, port)) {
        log("Failed to listen on port " + QString::number(port));
        return -1;
    }

    log("Listening on port " + QString::number(port));
    return app.exec();
}