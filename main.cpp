#include "qtwidgetsoctacq.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QtWidgetsOCTAcq w;
    //for(int i=0;i<100;i++)
    //    w.Logprint("Program begin...");

    w.show();
    return a.exec();
}
