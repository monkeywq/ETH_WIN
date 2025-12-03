#pragma once
#include <QObject>
#include "pcap.h" // ?? WinPcap/Npcap ???
class EthDevice:public QObject
{

    Q_OBJECT

public:
    //explicit EthernetDevice(QObject* parent = nullptr);
    //~EthernetDevice();

//    // 查找并返回可用网卡列表
//    QStringList findDevices();
//
//    // 打开指定网卡并开始捕获
//    bool openDevice(const QString& deviceName);
//
//    // 发送以太网帧
//    bool sendFrame(const QByteArray& frameData);
//
//    // 停止捕获
//    void closeDevice();
//
//signals:
//    // 接收到新帧时发射此信号
//    void frameReceived(const QByteArray& frame);
//    // 发生错误时发射此信号
//    void errorOccurred(const QString& message);
//
//private:
//    pcap_t* handle = nullptr; // 网卡句柄
//
//    // 捕获线程：由于 pcap_loop/pcap_next_ex 是阻塞的，需要放在独立线程
//    // 在 Qt 中通常使用 QThread 或直接在 QObject 中模拟线程循环
//    void startCaptureLoop();
};

