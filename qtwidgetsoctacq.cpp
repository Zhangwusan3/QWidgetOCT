#include "qtwidgetsoctacq.h"
#include <QScrollBar>
#include <qplaintextedit.h>

QtWidgetsOCTAcq::QtWidgetsOCTAcq(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
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
