#include "SoemMaster.h"
#include <QDebug>

// ==================== DM3E-556 PDO配置回调 ====================

// DM3E-556 CiA402 PDO映射配置（PRE-OP → SAFE-OP 阶段由SOEM自动调用）
int DM3E556_setup(uint16 slave)
{
    int retval = 0;
    uint8  u8val;
    uint16 u16val;
    uint32 u32val;

    // --- 配置 RxPDO (0x1600) ---
    // 先清空 RxPDO 映射条目数
    u8val = 0;
    retval += ec_SDOwrite(slave, 0x1600, 0x00, FALSE, sizeof(u8val), &u8val, EC_TIMEOUTRXM);

    // 映射 Controlword 0x6040:00, 16bit
    u32val = 0x60400010;
    retval += ec_SDOwrite(slave, 0x1600, 0x01, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    // 映射 Target Position 0x607A:00, 32bit
    u32val = 0x607A0020;
    retval += ec_SDOwrite(slave, 0x1600, 0x02, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    // 映射 Modes of Operation 0x6060:00, 8bit
    u32val = 0x60600008;
    retval += ec_SDOwrite(slave, 0x1600, 0x03, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTRXM);

    // 设置 RxPDO 映射条目数 = 3
    u8val = 3;
    retval += ec_SDOwrite(slave, 0x1600, 0x00, FALSE, sizeof(u8val), &u8val, EC_TIMEOUTRXM);

    // --- 配置 TxPDO (0x1A00) ---
    u8val = 0;
    retval += ec_SDOwrite(slave, 0x1A00, 0x00, FALSE, sizeof(u8val), &u8val, EC_TIMEOUTRXM);

    // 映射 Statusword 0x6041:00, 16bit
    u32val = 0x60410010;
    retval += ec_SDOwrite(slave, 0x1A00, 0x01, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    // 映射 Position Actual Value 0x6064:00, 32bit
    u32val = 0x60640020;
    retval += ec_SDOwrite(slave, 0x1A00, 0x02, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
    // 映射 Modes of Operation Display 0x6061:00, 8bit
    u32val = 0x60610008;
    retval += ec_SDOwrite(slave, 0x1A00, 0x03, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTRXM);

    u8val = 3;
    retval += ec_SDOwrite(slave, 0x1A00, 0x00, FALSE, sizeof(u8val), &u8val, EC_TIMEOUTRXM);

    // --- SM2 PDO分配 (RxPDO) 0x1C12 ---
    u8val = 0;
    retval += ec_SDOwrite(slave, 0x1C12, 0x00, FALSE, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
    u16val = 0x1600;
    retval += ec_SDOwrite(slave, 0x1C12, 0x01, FALSE, sizeof(u16val), &u16val, EC_TIMEOUTRXM);
    u8val = 1;
    retval += ec_SDOwrite(slave, 0x1C12, 0x00, FALSE, sizeof(u8val), &u8val, EC_TIMEOUTRXM);

    // --- SM3 PDO分配 (TxPDO) 0x1C13 ---
    u8val = 0;
    retval += ec_SDOwrite(slave, 0x1C13, 0x00, FALSE, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
    u16val = 0x1A00;
    retval += ec_SDOwrite(slave, 0x1C13, 0x01, FALSE, sizeof(u16val), &u16val, EC_TIMEOUTRXM);
    u8val = 1;
    retval += ec_SDOwrite(slave, 0x1C13, 0x00, FALSE, sizeof(u8val), &u8val, EC_TIMEOUTRXM);

    printf("DM3E-556 slave %d PDO configured, retval = %d\n", slave, retval);
    return 1;
}

// ==================== PdoCycleThread ====================

PdoCycleThread::PdoCycleThread(QObject *parent)
    : QThread(parent), m_running(false), m_cycleUs(1000)
    , m_phase(CIA402_FAULT_RESET), m_targetPos(0), m_posStep(10)
    , m_newSetpoint(false), m_phaseDelay(0)
{
}

void PdoCycleThread::stopCycle()
{
    m_running = false;
}

void PdoCycleThread::run()
{
    m_running = true;
    m_phase = CIA402_FAULT_RESET;
    m_phaseDelay = 0;
    m_newSetpoint = false;

    while (m_running)
    {
        ec_send_processdata();
        int wkc = ec_receive_processdata(EC_TIMEOUTRET);

        // 获取从站1的PDO指针
        DM3E_RxPDO *rxpdo = nullptr;
        DM3E_TxPDO *txpdo = nullptr;
        if (ec_slavecount >= 1)
        {
            if (ec_slave[1].outputs)
                rxpdo = (DM3E_RxPDO *)ec_slave[1].outputs;
            if (ec_slave[1].inputs)
                txpdo = (DM3E_TxPDO *)ec_slave[1].inputs;
        }

        if (rxpdo && txpdo)
        {
            uint16_t status = txpdo->statusword;

            // 始终设置为位置模式(PP mode = 1)
            rxpdo->modesOfOperation = 1;

            switch (m_phase)
            {
            case CIA402_FAULT_RESET:
                // 如果有故障(bit3=1)，先发故障复位(bit7=1)
                if (status & 0x0008)
                {
                    rxpdo->controlword = 0x0080; // Fault Reset
                }
                else
                {
                    rxpdo->controlword = 0x0000;
                }
                m_phaseDelay++;
                if (m_phaseDelay > 100 || !(status & 0x0008))
                {
                    m_phase = CIA402_SHUTDOWN;
                    m_phaseDelay = 0;
                }
                break;

            case CIA402_SHUTDOWN:
                // Shutdown命令: xxxx.xxxx.0xxx.0110
                rxpdo->controlword = 0x0006;
                // 等待 "Ready to Switch On": xxxx.xxxx.x01x.0001
                if ((status & 0x006F) == 0x0021)
                {
                    m_phase = CIA402_SWITCH_ON;
                    m_phaseDelay = 0;
                    // 初始化目标位置为当前实际位置
                    m_targetPos = txpdo->positionActualValue;
                }
                break;

            case CIA402_SWITCH_ON:
                // Switch On命令: xxxx.xxxx.0xxx.0111
                rxpdo->controlword = 0x0007;
                rxpdo->targetPosition = m_targetPos;
                // 等待 "Switched On": xxxx.xxxx.x01x.0011
                if ((status & 0x006F) == 0x0023)
                {
                    m_phase = CIA402_ENABLE_OP;
                    m_phaseDelay = 0;
                }
                break;

            case CIA402_ENABLE_OP:
                // Enable Operation命令: xxxx.xxxx.0xxx.1111
                rxpdo->controlword = 0x000F;
                rxpdo->targetPosition = m_targetPos;
                // 等待 "Operation Enabled": xxxx.xxxx.x01x.0111
                if ((status & 0x006F) == 0x0027)
                {
                    m_phase = CIA402_RUNNING;
                    m_phaseDelay = 0;
                    m_newSetpoint = false;
                }
                break;

            case CIA402_RUNNING:
            {
                // 位置模式运行中
                // bit10 (Target Reached) = 0x0400
                bool targetReached = (status & 0x0400) != 0;

                if (!m_newSetpoint)
                {
                    // 累加目标位置
                    m_targetPos += m_posStep;
                    rxpdo->targetPosition = m_targetPos;
                    // Enable Operation + New Setpoint(bit4) + Change immediately(bit5)
                    rxpdo->controlword = 0x003F;
                    m_newSetpoint = true;
                }
                else
                {
                    // 等目标到达后，清除bit4，准备下一次
                    if (targetReached)
                    {
                        rxpdo->controlword = 0x000F;
                        m_newSetpoint = false;
                    }
                    else
                    {
                        // 保持new setpoint
                        rxpdo->controlword = 0x003F;
                    }
                }
                break;
            }
            }
        }

        // 发送状态回UI
        QByteArray snapshot;
        if (txpdo)
            snapshot = QByteArray((const char *)txpdo, sizeof(DM3E_TxPDO));
        emit cycleData(wkc, snapshot);

        osal_usleep(m_cycleUs);
    }

    // 停止时发送 Disable Voltage
    if (ec_slavecount >= 1 && ec_slave[1].outputs)
    {
        DM3E_RxPDO *rxpdo = (DM3E_RxPDO *)ec_slave[1].outputs;
        rxpdo->controlword = 0x0000;
        ec_send_processdata();
        ec_receive_processdata(EC_TIMEOUTRET);
    }
}

// ==================== SoemMaster ====================

SoemMaster::SoemMaster(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_operational(false)
    , m_IOmapSize(0)
    , m_expectedWKC(0)
    , m_pdoThread(nullptr)
    , m_checkThread(nullptr)
    , m_checkRunning(false)
{
    memset(m_IOmap, 0, sizeof(m_IOmap));
}

SoemMaster::~SoemMaster()
{
    stopOp();
    close();
}

QStringList SoemMaster::enumAdapters()
{
    QStringList list;
    ec_adaptert *adapter = ec_find_adapters();
    ec_adaptert *head = adapter;
    while (adapter)
    {
        QString entry = QString("%1 | %2")
            .arg(QString::fromLocal8Bit(adapter->desc))
            .arg(QString::fromLocal8Bit(adapter->name));
        list.append(entry);
        adapter = adapter->next;
    }
    ec_free_adapters(head);
    return list;
}

bool SoemMaster::init(const QString &ifname)
{
    if (m_initialized)
        close();

    QByteArray name = ifname.toLocal8Bit();
    if (ec_init(name.data()))
    {
        m_initialized = true;
        emit logMessage(QString("ec_init(%1) 成功").arg(ifname));
        return true;
    }
    emit logMessage(QString("ec_init(%1) 失败").arg(ifname));
    return false;
}

int SoemMaster::scanSlaves()
{
    if (!m_initialized)
        return 0;

    int cnt = ec_config_init(FALSE);
    if (cnt > 0)
    {
        emit logMessage(QString("发现 %1 个从站").arg(ec_slavecount));
        for (int i = 1; i <= ec_slavecount; i++)
        {
            emit logMessage(QString("  从站%1: %2 (VendorID=0x%3 ProductCode=0x%4)")
                .arg(i)
                .arg(QString::fromLocal8Bit(ec_slave[i].name))
                .arg(ec_slave[i].eep_man, 8, 16, QChar('0'))
                .arg(ec_slave[i].eep_id, 8, 16, QChar('0')));
        }
    }
    else
    {
        emit logMessage("未发现从站");
    }
    return ec_slavecount;
}

QVector<SlaveInfo> SoemMaster::getSlaveInfoList() const
{
    QVector<SlaveInfo> list;
    for (int i = 1; i <= ec_slavecount; i++)
    {
        SlaveInfo info;
        info.index = i;
        info.name = QString::fromLocal8Bit(ec_slave[i].name);
        info.vendorId = ec_slave[i].eep_man;
        info.productCode = ec_slave[i].eep_id;
        info.state = ec_slave[i].state;
        info.obytes = ec_slave[i].Obytes;
        info.ibytes = ec_slave[i].Ibytes;
        list.append(info);
    }
    return list;
}

bool SoemMaster::startOp(int cycleUs)
{
    if (!m_initialized || m_operational)
        return false;

    // 为所有从站注册PDO配置回调（DM3E-556）
    for (int slc = 1; slc <= ec_slavecount; slc++)
    {
        ec_slave[slc].PO2SOconfig = &DM3E556_setup;
        emit logMessage(QString("从站%1: 注册DM3E-556 PDO配置").arg(slc));
    }

    // IO映射
    m_IOmapSize = ec_config_map(&m_IOmap);
    emit logMessage(QString("IOmap大小: %1 字节").arg(m_IOmapSize));

    // 配置DC
    ec_configdc();

    // 等待SAFE_OP
    ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);
    if (ec_slave[0].state != EC_STATE_SAFE_OP)
    {
        emit logMessage("从站未能进入SAFE_OP状态");
        return false;
    }
    emit logMessage("所有从站已进入SAFE_OP");

    m_expectedWKC = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC;
    emit logMessage(QString("期望WKC: %1").arg(m_expectedWKC));

    // 先发一次PDO
    ec_send_processdata();
    ec_receive_processdata(EC_TIMEOUTRET);

    // 请求OP
    ec_slave[0].state = EC_STATE_OPERATIONAL;
    ec_writestate(0);

    // 等待OP
    int chk = 200;
    do
    {
        ec_statecheck(0, EC_STATE_OPERATIONAL, 50000);
    } while (chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));

    if (ec_slave[0].state == EC_STATE_OPERATIONAL)
    {
        m_operational = true;
        // 刷新各从站状态缓存，使getSlaveInfoList读到最新值
        ec_readstate();
        emit logMessage("所有从站已进入OPERATIONAL");

        // 启动ecatcheck线程
        m_checkRunning = true;
        osal_thread_create(&m_checkThread, 128000, (void*)&SoemMaster::ecatCheck, this);

        // 启动PDO循环线程
        m_pdoThread = new PdoCycleThread(this);
        m_pdoThread->setCycleTimeUs(cycleUs);
        connect(m_pdoThread, &PdoCycleThread::cycleData, this, &SoemMaster::onCycleData);
        m_pdoThread->start();

        return true;
    }

    emit logMessage("从站未能进入OPERATIONAL状态");
    // 打印各从站状态
    ec_readstate();
    for (int i = 1; i <= ec_slavecount; i++)
    {
        if (ec_slave[i].state != EC_STATE_OPERATIONAL)
        {
            emit logMessage(QString("  从站%1 State=0x%2 ALStatus=0x%3: %4")
                .arg(i)
                .arg(ec_slave[i].state, 2, 16, QChar('0'))
                .arg(ec_slave[i].ALstatuscode, 4, 16, QChar('0'))
                .arg(ec_ALstatuscode2string(ec_slave[i].ALstatuscode)));
        }
    }
    return false;
}

void SoemMaster::stopOp()
{
    if (m_pdoThread)
    {
        m_pdoThread->stopCycle();
        m_pdoThread->wait(3000);
        delete m_pdoThread;
        m_pdoThread = nullptr;
    }

    m_checkRunning = false;
    // 给ecatcheck线程一点时间退出
    osal_usleep(20000);

    if (m_operational)
    {
        ec_slave[0].state = EC_STATE_INIT;
        ec_writestate(0);
        m_operational = false;
        emit logMessage("已请求从站回到INIT状态");
    }
}

void SoemMaster::close()
{
    stopOp();
    if (m_initialized)
    {
        ec_close();
        m_initialized = false;
        emit logMessage("主站已关闭");
    }
}

void SoemMaster::onCycleData(int wkc, const QByteArray &ioData)
{
    Q_UNUSED(ioData);
    emit pdoCycleUpdate(wkc);
}

DM3E_RxPDO *SoemMaster::getRxPDO(int slave)
{
    if (slave < 1 || slave > ec_slavecount || !ec_slave[slave].outputs)
        return nullptr;
    return (DM3E_RxPDO *)ec_slave[slave].outputs;
}

DM3E_TxPDO *SoemMaster::getTxPDO(int slave)
{
    if (slave < 1 || slave > ec_slavecount || !ec_slave[slave].inputs)
        return nullptr;
    return (DM3E_TxPDO *)ec_slave[slave].inputs;
}

// 从站状态检查线程（参照simple_test中的ecatcheck）
OSAL_THREAD_FUNC SoemMaster::ecatCheck(void *param)
{
    SoemMaster *self = (SoemMaster *)param;

    while (self->m_checkRunning)
    {
        if (self->m_operational && (ec_group[0].docheckstate))
        {
            ec_group[0].docheckstate = FALSE;
            ec_readstate();
            for (int slave = 1; slave <= ec_slavecount; slave++)
            {
                if (ec_slave[slave].state != EC_STATE_OPERATIONAL)
                {
                    if (ec_slave[slave].state == (EC_STATE_SAFE_OP + EC_STATE_ERROR))
                    {
                        ec_slave[slave].state = (EC_STATE_SAFE_OP + EC_STATE_ACK);
                        ec_writestate(slave);
                    }
                    else if (ec_slave[slave].state == EC_STATE_SAFE_OP)
                    {
                        ec_slave[slave].state = EC_STATE_OPERATIONAL;
                        ec_writestate(slave);
                    }
                    else if (ec_slave[slave].state > EC_STATE_NONE)
                    {
                        if (ec_reconfig_slave(slave, 500))
                        {
                            ec_slave[slave].islost = FALSE;
                        }
                    }
                    else if (!ec_slave[slave].islost)
                    {
                        ec_statecheck(slave, EC_STATE_OPERATIONAL, EC_TIMEOUTRET);
                        if (ec_slave[slave].state == EC_STATE_NONE)
                        {
                            ec_slave[slave].islost = TRUE;
                        }
                    }
                }
                if (ec_slave[slave].islost)
                {
                    if (ec_slave[slave].state == EC_STATE_NONE)
                    {
                        ec_recover_slave(slave, 500);
                    }
                    else
                    {
                        ec_slave[slave].islost = FALSE;
                    }
                }
            }
        }
        osal_usleep(10000);
    }
}
