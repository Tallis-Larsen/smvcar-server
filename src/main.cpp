#include <QCoreApplication>
#include <QHttpServer>
#include <QHttpServerResponse>
#include <QFile>
#include "Server.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    Server server;

    return app.exec();
}