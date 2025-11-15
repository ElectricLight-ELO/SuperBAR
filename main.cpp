#include "superBAR.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    superBAR window;
    window.show();
    return app.exec();
}
