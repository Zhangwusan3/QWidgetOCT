#include "qtwidgetsoctacq.h"
#include <QtWidgets/QApplication>
#include "ATSdigitizer.h"
#include "stdio.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QtWidgetsOCTAcq w;
    w.Logprint("Program begin...");
    w.show();
    
    ATSdigitizer d;
   
    w.Logprint("Digitizer init...");
    d.AlazarDLLCfg();
    w.Logprint("Digitizer cfg...");
    d.AlazarDLLAcq();
    w.Logprint("Digitizer acq...");
    
    w.Logprint("Program begin2...");
    a.exec();
    w.Logprint("Program begin3...");
    w.show();

    return 1;
}
int main01(int argc, char* argv[]) {
    printf("Hello...");
    ATSdigitizer a;
    a.AlazarDLLCfg();
    a.AlazarDLLAcq();
    getchar();
    return 1;
}
