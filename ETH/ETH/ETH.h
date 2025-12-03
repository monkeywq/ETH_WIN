#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_ETH.h"

#include <QMainWindow>
#include <QThread>
#include <QListWidgetItem>
#include <QDateTime>
#include <QMessageBox>
#include <QDebug>

// WinPcap 头文件
#include <pcap.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <ntddndis.h>
#include <QTimer>

// 链接依赖库（VS 也可通过属性页配置，此处为兼容写法）
#pragma comment(lib, "wpcap.lib")
#pragma comment(lib, "packet.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
// 捕获线程（避免阻塞UI）
class CaptureThread : public QThread
{
    Q_OBJECT
public:
    explicit CaptureThread(pcap_t* adhandle, QObject* parent = nullptr);
    ~CaptureThread() override;
    void stopCapture();

signals:
    void packetCaptured(const QString& info, const QByteArray& data);

protected:
    void run() override;

private:
    pcap_t* m_adhandle;
    volatile bool m_isRunning;



};
typedef struct  {
    u_char mac[6];
}MAC_ADDR; 
typedef struct
{
    MAC_ADDR MacAddr;
    QString DeviceName;
}ETH_DEVICE;

class ETH : public QMainWindow
{
    Q_OBJECT

public:
    ETH(QWidget *parent = nullptr);
    //void SendPacket();
    void SendPacket(int DeviceIdx, u_char* pData, int Length);
    ~ETH();
private slots:
    void on_deviceListWidget_itemClicked(QString DeviceName);
    void SlotSendETH_Packet();
   
    //void on_startCaptureBtn_clicked();
    //void on_stopCaptureBtn_clicked();
    //void on_sendBtn_clicked();
    //void handleCapturedPacket(const QString& info, const QByteArray& data);
private:
   QVector<ETH_DEVICE>ETH_Device;
   QTimer timer;
   unsigned char* packet;     // 完整数据包指针
   int packet_size;//
private:
    Ui::ETHClass ui;
    pcap_if_t* m_alldevs;
    pcap_t* m_adhandle;
    CaptureThread* m_captureThread;
    char m_errbuf[PCAP_ERRBUF_SIZE];
    void enumNetworkDevices();                  // 枚举网卡
    QString formatMacAddress(const u_char* mac); // 格式化MAC地址
    bool getDeviceMac(QString devName, u_char* mac); // 获取设备MAC地址（VS环境适配）
    QString formatPacketData(const QByteArray& data); // 格式化数据包
};



