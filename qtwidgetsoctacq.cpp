#include "qtwidgetsoctacq.h"
#include <QScrollBar>
#include <qplaintextedit.h>
#include "NIDAQmx.h"
#include "ATSdigitizer.h"

QtWidgetsOCTAcq::QtWidgetsOCTAcq(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    connect(ui.pushButton, SIGNAL(clicked()), this, SLOT(on_pushAcq()));
}

QtWidgetsOCTAcq::~QtWidgetsOCTAcq()
{}

void QtWidgetsOCTAcq::Logprint(QString strlog)
{   

    QPlainTextEdit* plainTextEdit_log = ui.plainTextEdit;
    /* ineffective
    QTextCursor textCursor = plainTextEdit_log->textCursor();
    QTextBlockFormat textBlockFormat;
    textBlockFormat.setLineHeight(5, QTextBlockFormat::FixedHeight);//set line spacing
    textCursor.setBlockFormat(textBlockFormat);
    plainTextEdit_log->setTextCursor(textCursor);
    */

    //read only
    if (!plainTextEdit_log->isReadOnly())
    {
        plainTextEdit_log->setReadOnly(true);
    }

    //move cursor to end of the text
    plainTextEdit_log->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
    //clear the text when the content is full
    if (plainTextEdit_log->toPlainText().size() > 1024 * 4)
    {
        plainTextEdit_log->clear();
    }
    plainTextEdit_log->appendPlainText(strlog);
    //plainTextEdit_log->insertPlainText(strlog);
    //move scrollbar to the bottom
    QScrollBar* scrollbar = plainTextEdit_log->verticalScrollBar();
    if (scrollbar)
    {
        scrollbar->setSliderPosition(scrollbar->maximum());
    }

}

void QtWidgetsOCTAcq::on_pushAcq() {
    ui.pushButton->setText(tr("Acquire data"));
    ATSdigitizer d;

    Logprint("Digitizer init...");
    d.AlazarDLLCfg();
    Logprint("Digitizer cfg...");
    d.AlazarDLLAcq();
    Logprint("Digitizer acq...");
    int         error = 0;
    TaskHandle	taskHandle = 0;
    uInt32      data = 0xffffffff;
    char        errBuff[2048] = { '\0' };
    int32		written;
    data = 0;
    DAQmxCreateTask("", &taskHandle);
    DAQmxCreateDOChan(taskHandle, "Dev1/port0/line2", "", DAQmx_Val_ChanForAllLines);
    DAQmxStartTask(taskHandle);
    DAQmxWriteDigitalU32(taskHandle, 1, 1, 10.0, DAQmx_Val_GroupByChannel, &data, &written, NULL);
    data = 0xffffffff;
    DAQmxWriteDigitalU32(taskHandle, 1, 1, 10.0, DAQmx_Val_GroupByChannel, &data, &written, NULL);
}