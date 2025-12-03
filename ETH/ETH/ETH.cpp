#include "ETH.h"
//#include <netinet/if_ether.h>  // Ethernet头文件
// ====================== CaptureThread 实现 ======================
CaptureThread::CaptureThread(pcap_t* adhandle, QObject* parent)
    : QThread(parent), m_adhandle(adhandle), m_isRunning(false)
{}
CaptureThread::~CaptureThread()
{
    stopCapture();
    wait();
}
void CaptureThread::stopCapture()
{
    m_isRunning = false;
}

void CaptureThread::run()
{
    m_isRunning = true;
    struct pcap_pkthdr* header;
    const u_char* pkt_data;
    int res;

    while (m_isRunning)
    {
        res = pcap_next_ex(m_adhandle, &header, &pkt_data);
        if (res == 0) continue;  // 超时
        if (res == -1 || res == -2) break; // 错误或中断

        // 构造数据包信息
        QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        QString info = QString("[%1] 接收数据包 长度: %2 bytes")
            .arg(timeStr)
            .arg(header->len);

        emit packetCaptured(info, QByteArray((const char*)pkt_data, header->len));
    }
}
ETH::ETH(QWidget *parent)
    : QMainWindow(parent)
{
   // ui.setupUi(this);
    ui.setupUi(this);
    setWindowTitle("VS Qt WinPcap ETH Demo");
    // 枚举网卡
    enumNetworkDevices();
    m_alldevs = nullptr;
    timer.setInterval(100);





    //SendPacket(1,packet, packet_size);
    // 初始化UI
    //ui->stopCaptureBtn->setEnabled(false);
    //ui->sendBtn->setEnabled(false);
    //ui->recvTextEdit->setReadOnly(true);
    connect(ui.deviceListWidget, SIGNAL(currentIndexChanged(QString)), this, SLOT(on_deviceListWidget_itemClicked(QString)));
    connect(&timer, &QTimer::timeout, this, &ETH::SlotSendETH_Packet);
    timer.start();
}
ETH::~ETH()
{}
// 枚举所有网络设备（适配VS的WinPcap调用）
void ETH::enumNetworkDevices()
{
    //ui.deviceListWidget->clear();
    ui.deviceListWidget->clear();
    // 获取设备列表
    pcap_if_t* alldevs;
    char errbuf[PCAP_ERRBUF_SIZE];
    //pcap_findalldevs(&alldevs, errbuf);
    if (pcap_findalldevs(&alldevs, errbuf) == -1)
    {
        QMessageBox::critical(this, "error", QString("error: %1").arg(m_errbuf));
        return;
    }
    // 遍历设备
    pcap_if_t* d;
    int idx = 0;
    for (d = alldevs; d; d = d->next, idx++)
    {
        QString devDesc = d->description ? d->description : "无描述信息";
        ETH_DEVICE device;
        QString devName = d->name;
        MAC_ADDR MacAddr;
        device.DeviceName = devName;
        //MacAddrs.push_back(MacAddr);
        getDeviceMac(devName, (u_char*) (& MacAddr));
        device.MacAddr = MacAddr;
        QString MacStr = formatMacAddress((u_char*)(& MacAddr));
        ETH_Device.push_back(device);
        // 显示设备信息（包含索引，方便选择）
        //QString DeviceInfo = QString("[%1] DeviceIntroduction: %2\n  DeviceName: %3  MAC: %4")
        //    .arg(idx + 1)
        //    .arg(devDesc)
        //    .arg(devName)
        //    .arg(MacStr);
        QString DeviceInfo = QString("devcie%1").arg(QString::number(idx)); //devName;//QString("DeviceName: %1").arg(devName);
        QListWidgetItem* item = new QListWidgetItem(DeviceInfo); 
        qDebug() << "MAC:" << MacStr;
        qDebug() << DeviceInfo;
        item->setData(Qt::UserRole, QVariant::fromValue((void*)d));
        ui.deviceListWidget->addItem(DeviceInfo);
    }
    if (idx == 0)
    {
        QMessageBox::warning(this, "警告", "未找到网络设备，请确认已安装WinPcap并重启VS");
    }
}
// 格式化MAC地址（6字节转字符串）
QString ETH::formatMacAddress(const u_char* mac)
{
    return QString("%1:%2:%3:%4:%5:%6")
        .arg(mac[0], 2, 16, QChar('0'))
        .arg(mac[1], 2, 16, QChar('0'))
        .arg(mac[2], 2, 16, QChar('0'))
        .arg(mac[3], 2, 16, QChar('0'))
        .arg(mac[4], 2, 16, QChar('0'))
        .arg(mac[5], 2, 16, QChar('0'))
        .toUpper();
}
// 获取设备MAC地址（VS环境下通过IPHLPAPI实现，更稳定）
bool ETH::getDeviceMac(QString devName, u_char* mac)
{
    // 从设备名称中提取GUID（WinPcap设备名格式：\\Device\\NPF_{GUID}）
    QString devNameStr = devName;
    int guidStart = devNameStr.indexOf("{");
    int guidEnd = devNameStr.indexOf("}");
    if (guidStart == -1 || guidEnd == -1) return false;

    QString guid = devNameStr.mid(guidStart, guidEnd - guidStart + 1);

    // 通过IPHLPAPI获取适配器信息
    ULONG outBufLen = sizeof(IP_ADAPTER_INFO);
    PIP_ADAPTER_INFO pAdapterInfo = (PIP_ADAPTER_INFO)malloc(outBufLen);

    if (GetAdaptersInfo(pAdapterInfo, &outBufLen) == ERROR_BUFFER_OVERFLOW)
    {
        free(pAdapterInfo);
        pAdapterInfo = (PIP_ADAPTER_INFO)malloc(outBufLen);
        if (!pAdapterInfo) return false;
    }

    bool found = false;
    if (GetAdaptersInfo(pAdapterInfo, &outBufLen) == NO_ERROR)
    {
        PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
        while (pAdapter)
        {
            // 匹配GUID
            if (QString(pAdapter->AdapterName) == guid)
            {
                // 复制MAC地址（6字节）
                memcpy(mac, pAdapter->Address, 6);
                found = true;
                break;
            }
            pAdapter = pAdapter->Next;
        }
    }
    free(pAdapterInfo);
    return found;
}
// 格式化数据包（十六进制+ASCII，便于查看）
QString ETH::formatPacketData(const QByteArray& data)
{
    QString hexStr, asciiStr;
    for (int i = 0; i < data.size(); i++)
    {
        u_char c = (u_char)data.at(i);
        hexStr += QString("%1 ").arg(c, 2, 16, QChar('0')).toUpper();
        asciiStr += (c >= 0x20 && c <= 0x7E) ? QChar(c) : '.';

        // 每16字节换行，对齐显示
        if ((i + 1) % 16 == 0)
        {
            hexStr += "  |  " + asciiStr + "\n";
            asciiStr.clear();
        }
    }
    // 处理最后一行不足16字节的情况
    if (!asciiStr.isEmpty())
    {
        int spaceCount = (16 - data.size() % 16) * 3;
        hexStr += QString(" ").repeated(spaceCount) + "  |  " + asciiStr + "\n";
    }
    return hexStr;
}

// 选择网卡（点击列表项触发）
void ETH::on_deviceListWidget_itemClicked(QString DeviceName)
{
    // 停止当前捕获
    if (m_captureThread && m_captureThread->isRunning())
    {
        m_captureThread->stopCapture();
        m_captureThread->wait();
    }
    // 关闭旧设备句柄
    if (m_adhandle)
    {
        pcap_close(m_adhandle);
        m_adhandle = nullptr;
    }

    // 获取选中的设备
    //pcap_if_t* dev = (pcap_if_t*)item->data(Qt::UserRole).value<void*>();
    //if (!dev) return;

    // 打开设备（混杂模式，超时100ms，捕获长度65536字节）
    
    m_adhandle = pcap_open_live(
        DeviceName.toLatin1().data(),  //QString 转 char*
        65536,
        1,
        100,
        m_errbuf
    );
    if (!m_adhandle)
    {
        QMessageBox::critical(this, "Error", QString("open faile: %1").arg(m_errbuf));
        //ui.sendBtn->setEnabled(false);
        return;
    }
    QMessageBox::about(this, "OK", "Open Success");
    //// 验证是否为以太网设备（DLT_EN10MB = 以太网）
    //if (pcap_datalink(m_adhandle) != DLT_EN10MB)
    //{
    //    QMessageBox::critical(this, "错误", "选中的设备不是以太网设备（仅支持有线网卡）");
    //    pcap_close(m_adhandle);
    //    m_adhandle = nullptr;
    //    ui.sendBtn->setEnabled(false);
    //    return;
    //}

    //// 显示选中状态
    //ui.statusBar->showMessage(QString("已选中: %1").arg(dev->description ? dev->description : dev->name));
    //ui.sendBtn->setEnabled(true);
}
void ETH::SendPacket(int DeviceIdx,u_char* pData,int Length)
{
    QString SendData = "SendData:";
    for (int i = 0; i < Length; i++)
    {
        SendData += QString::number(pData[i], 16);
        SendData += " ";
    }
    qDebug() << SendData;
    //on_deviceListWidget_itemClicked(ui.deviceListWidget->itemText())
    pcap_t* handle;
    char errbuf[PCAP_ERRBUF_SIZE];
    handle = pcap_open_live(
        ETH_Device[DeviceIdx].DeviceName.toLatin1().data(),  //QString 转 char*
        65536,
        1,
        100,
        errbuf);
    if (!handle)
    {
        QMessageBox::critical(this, "Error", QString("open faile: %1").arg(m_errbuf));
    }
    else
    {
        //QMessageBox::about(this, "OK", "Open Success");
    }

    if (pcap_sendpacket(handle, pData, Length) != 0) {   // 发送数据包
        //fprintf(stderr, "\nError sending the packet: \n", pcap_geterr(handle));
        QMessageBox::about(this, "Error", "Send Packet Error");
        //return -1;
    }
    else
    {
        free(packet);
        pcap_close(handle);
       // QMessageBox::about(this, "OK", "Send Packet OK");
    }
    //if (pcap_sendpacket(handle, packet, packet_size) != 0) {   // 发送数据包
    //    fprintf(stderr, "\nError sending the packet: \n", pcap_geterr(handle));
    //    return -1;
    //}

}
void ETH::SlotSendETH_Packet()
{
    unsigned char SrcMac[6] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 }; // 源MAC地址
    unsigned char DstMac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }; // 目标MAC地址
    unsigned char DataType[2] = { 0x88,0xA4 };  //帧类型

    packet_size = 14 + 80;// sizeof(char)* strlen("Hello World"); // 数据包总大小
    packet = (unsigned char*)malloc(packet_size);   // 分配内存空间
    memset(packet, 0, packet_size);
    memcpy(packet, DstMac, 6);
    memcpy(packet + 6, &(ETH_Device[1].MacAddr), 6);
    packet[12] = 0x88;
    packet[13] = 0xA4;
    SendPacket(1,packet,packet_size);
}