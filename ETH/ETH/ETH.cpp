#include "ETH.h"
#include <QMessageBox>

ETH::ETH(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    m_master = new SoemMaster(this);

    // 枚举网卡
    QStringList adapters = m_master->enumAdapters();
    for (const QString &a : adapters)
        ui.comboAdapter->addItem(a);

    // 信号连接
    connect(ui.btnInit,    &QPushButton::clicked, this, &ETH::onBtnInit);
    connect(ui.btnScan,    &QPushButton::clicked, this, &ETH::onBtnScan);
    connect(ui.btnStartOp, &QPushButton::clicked, this, &ETH::onBtnStartOp);
    connect(ui.btnStopOp,  &QPushButton::clicked, this, &ETH::onBtnStopOp);

    connect(m_master, &SoemMaster::logMessage, this, &ETH::onLogMessage);
    connect(m_master, &SoemMaster::pdoCycleUpdate, this, &ETH::onPdoCycleUpdate);

    ui.tableSlaves->horizontalHeader()->setStretchLastSection(true);
}

ETH::~ETH()
{
    m_master->close();
}

void ETH::onBtnInit()
{
    if (ui.comboAdapter->currentIndex() < 0)
    {
        QMessageBox::warning(this, "警告", "请先选择网卡");
        return;
    }

    // 从 "描述 | 设备名" 格式中提取设备名
    QString text = ui.comboAdapter->currentText();
    int sep = text.indexOf(" | ");
    m_selectedIfname = (sep >= 0) ? text.mid(sep + 3) : text;

    if (m_master->init(m_selectedIfname))
    {
        ui.btnScan->setEnabled(true);
        ui.btnInit->setEnabled(false);
        ui.comboAdapter->setEnabled(false);
    }
}

void ETH::onBtnScan()
{
    int cnt = m_master->scanSlaves();
    if (cnt > 0)
    {
        updateSlaveTable();
        ui.btnStartOp->setEnabled(true);
    }
}

void ETH::onBtnStartOp()
{
    if (m_master->startOp(1000))
    {
        updateSlaveTable();
        ui.btnStartOp->setEnabled(false);
        ui.btnStopOp->setEnabled(true);
        ui.btnScan->setEnabled(false);
    }
    else
    {
        updateSlaveTable();
    }
}

void ETH::onBtnStopOp()
{
    m_master->stopOp();
    ui.btnStopOp->setEnabled(false);
    ui.btnStartOp->setEnabled(true);
    ui.btnScan->setEnabled(true);
    ui.labelWKC->setText("WKC: --");
    updateSlaveTable();
}

void ETH::onLogMessage(const QString &msg)
{
    ui.textLog->append(msg);
}

void ETH::onPdoCycleUpdate(int wkc)
{
    ui.labelWKC->setText(QString("WKC: %1").arg(wkc));

    // 显示DM3E-556 TxPDO数据
    DM3E_TxPDO *tx = m_master->getTxPDO(1);
    if (tx)
    {
        QString cia402State;
        uint16_t sw = tx->statusword;
        if      ((sw & 0x004F) == 0x0000) cia402State = "Not Ready";
        else if ((sw & 0x004F) == 0x0040) cia402State = "Switch On Disabled";
        else if ((sw & 0x006F) == 0x0021) cia402State = "Ready to Switch On";
        else if ((sw & 0x006F) == 0x0023) cia402State = "Switched On";
        else if ((sw & 0x006F) == 0x0027) cia402State = "Operation Enabled";
        else if ((sw & 0x006F) == 0x0007) cia402State = "Quick Stop Active";
        else if ((sw & 0x004F) == 0x000F) cia402State = "Fault Reaction";
        else if ((sw & 0x004F) == 0x0008) cia402State = "Fault";
        else cia402State = "Unknown";

        bool targetReached = (sw & 0x0400) != 0;

        ui.labelStatus->setText(
            QString("[%1] SW:0x%2  Pos:%3  Target:%4  Mode:%5")
            .arg(cia402State)
            .arg(sw, 4, 16, QChar('0'))
            .arg(tx->positionActualValue)
            .arg(targetReached ? "Reached" : "Moving")
            .arg(tx->modesOfOperationDisplay));
    }
}

void ETH::updateSlaveTable()
{
    QVector<SlaveInfo> slaves = m_master->getSlaveInfoList();
    ui.tableSlaves->setRowCount(slaves.size());

    for (int i = 0; i < slaves.size(); i++)
    {
        const SlaveInfo &s = slaves[i];
        ui.tableSlaves->setItem(i, 0, new QTableWidgetItem(QString::number(s.index)));
        ui.tableSlaves->setItem(i, 1, new QTableWidgetItem(s.name));
        ui.tableSlaves->setItem(i, 2, new QTableWidgetItem(QString("0x%1").arg(s.vendorId, 8, 16, QChar('0'))));
        ui.tableSlaves->setItem(i, 3, new QTableWidgetItem(QString("0x%1").arg(s.productCode, 8, 16, QChar('0'))));
        ui.tableSlaves->setItem(i, 4, new QTableWidgetItem(stateToString(s.state)));
        ui.tableSlaves->setItem(i, 5, new QTableWidgetItem(QString("O:%1 / I:%2").arg(s.obytes).arg(s.ibytes)));
    }
}

QString ETH::stateToString(uint16_t state)
{
    switch (state & 0x0F)
    {
    case 0x01: return "INIT";
    case 0x02: return "PRE-OP";
    case 0x03: return "BOOT";
    case 0x04: return "SAFE-OP";
    case 0x08: return "OPERATIONAL";
    default:   return QString("0x%1").arg(state, 2, 16, QChar('0'));
    }
}
