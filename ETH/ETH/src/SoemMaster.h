#pragma once

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QVector>
#include <QString>

extern "C" {
#include "ethercat.h"
}

// 从站信息结构
struct SlaveInfo {
    int index;
    QString name;
    uint32_t vendorId;
    uint32_t productCode;
    uint16_t state;
    int obytes;
    int ibytes;
};

// DM3E-556 CiA402 RxPDO 结构（主站→从站）
#pragma pack(push, 1)
struct DM3E_RxPDO {
    uint16_t controlword;      // 0x6040
    int32_t  targetPosition;   // 0x607A
    int8_t   modesOfOperation; // 0x6060
};

// DM3E-556 CiA402 TxPDO 结构（从站→主站）
struct DM3E_TxPDO {
    uint16_t statusword;              // 0x6041
    int32_t  positionActualValue;     // 0x6064
    int8_t   modesOfOperationDisplay; // 0x6061
};
#pragma pack(pop)

// CiA402 状态机阶段
enum CIA402_Phase {
    CIA402_FAULT_RESET,
    CIA402_SHUTDOWN,
    CIA402_SWITCH_ON,
    CIA402_ENABLE_OP,
    CIA402_RUNNING
};

// PDO循环线程（含CiA402状态机）
class PdoCycleThread : public QThread
{
    Q_OBJECT
public:
    explicit PdoCycleThread(QObject *parent = nullptr);
    void stopCycle();
    void setCycleTimeUs(int us) { m_cycleUs = us; }
    void setPositionStep(int32_t step) { m_posStep = step; }

signals:
    void cycleData(int wkc, const QByteArray &ioData);

protected:
    void run() override;

private:
    volatile bool m_running;
    int m_cycleUs;
    CIA402_Phase m_phase;
    int32_t m_targetPos;
    int32_t m_posStep;
    bool m_newSetpoint;    // 用于翻转bit4
    int m_phaseDelay;      // 状态切换延时计数
};

// SOEM主站控制类
class SoemMaster : public QObject
{
    Q_OBJECT
public:
    explicit SoemMaster(QObject *parent = nullptr);
    ~SoemMaster();

    // 枚举可用网卡
    QStringList enumAdapters();

    // 初始化主站（绑定网卡）
    bool init(const QString &ifname);

    // 扫描从站
    int scanSlaves();

    // 获取从站信息列表
    QVector<SlaveInfo> getSlaveInfoList() const;

    // 切换到OP状态，开始PDO循环
    bool startOp(int cycleUs = 1000);

    // 停止PDO循环，回到INIT
    void stopOp();

    // 关闭主站
    void close();

    bool isInitialized() const { return m_initialized; }
    bool isOperational() const { return m_operational; }
    int slaveCount() const { return ec_slavecount; }

    // 获取IOMap指针（供外部读写PDO）
    char *ioMap() { return m_IOmap; }
    int ioMapSize() const { return m_IOmapSize; }

    // DM3E-556 PDO 访问
    DM3E_RxPDO *getRxPDO(int slave = 1);
    DM3E_TxPDO *getTxPDO(int slave = 1);

signals:
    void logMessage(const QString &msg);
    void slaveStateChanged(int slave, uint16_t state);
    void pdoCycleUpdate(int wkc);

private slots:
    void onCycleData(int wkc, const QByteArray &ioData);

private:
    bool m_initialized;
    bool m_operational;
    char m_IOmap[4096];
    int m_IOmapSize;
    int m_expectedWKC;

    PdoCycleThread *m_pdoThread;

    // ecatcheck 线程
    static OSAL_THREAD_FUNC ecatCheck(void *param);
    OSAL_THREAD_HANDLE m_checkThread;
    volatile bool m_checkRunning;
};
