#include <QCoreApplication>
#include "Server.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    Server server;

    return app.exec();
}