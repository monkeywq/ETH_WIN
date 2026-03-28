#pragma once

#include <QMainWindow>
#include <QHeaderView>
#include "ui_ETH.h"
#include "SoemMaster.h"

class ETH : public QMainWindow
{
    Q_OBJECT

public:
    ETH(QWidget *parent = nullptr);
    ~ETH();

private slots:
    void onBtnInit();
    void onBtnScan();
    void onBtnStartOp();
    void onBtnStopOp();
    void onLogMessage(const QString &msg);
    void onPdoCycleUpdate(int wkc);

private:
    Ui::ETHClass ui;
    SoemMaster *m_master;
    QString m_selectedIfname;

    void updateSlaveTable();
    QString stateToString(uint16_t state);
};
