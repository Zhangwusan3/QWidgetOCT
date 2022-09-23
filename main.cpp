#include "qtwidgetsoctacq.h"
#include <QtWidgets/QApplication>
#include "ATSdigitizer.h"
#include "stdio.h"
#include "NIDAQmx.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QtWidgetsOCTAcq w;
    w.Logprint("Program begin...");
    w.show();
    w.Logprint("NIdaq init");
    int         error = 0;
    TaskHandle  taskHandle = 0;
    char        errBuff[2048] = { '\0' };

    DAQmxCreateTask("", &taskHandle);
    DAQmxCreateCOPulseChanFreq(taskHandle, "Dev1/ctr0", "", DAQmx_Val_Hz, DAQmx_Val_Low, 0.0, 1000.0, 0.5);
    DAQmxConnectTerms("Dev1/ctr0", "Dev1/PFI3", DAQmx_Val_DoNotInvertPolarity);
    DAQmxCfgDigEdgeStartTrig(taskHandle, "/Dev1/PFI1", DAQmx_Val_Rising);
    DAQmxCfgImplicitTiming(taskHandle, DAQmx_Val_ContSamps, 1000);
    w.Logprint("NIdaq config");
    //DAQmxRegisterDoneEvent(taskHandle, 0, DoneCallback, NULL);
    DAQmxStartTask(taskHandle); 

    a.exec();
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
