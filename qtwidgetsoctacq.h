#pragma once

#include <QtWidgets/QWidget>
#include "ui_qtwidgetsoctacq.h"
#include <QString>
#include <QString>

class QtWidgetsOCTAcq : public QWidget
{
    Q_OBJECT

public:
    QtWidgetsOCTAcq(QWidget *parent = nullptr);
    ~QtWidgetsOCTAcq();

public:
    void Logprint(QString strlog);

private:
    Ui::QtWidgetsOCTAcqClass ui;
    
public slots:
    void on_pushAcq();
};
