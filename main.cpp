#include "interfacemanager.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    InterfaceManager w;
    w.show();
    return a.exec();
}
