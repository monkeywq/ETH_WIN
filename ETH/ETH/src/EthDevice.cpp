#include "EthDevice.h"
#include <QDebug>



//QStringList EthernetDevice::findDevices()
//{
//    pcap_if_t* alldevs;
//    pcap_if_t* d;
//    char errbuf[PCAP_ERRBUF_SIZE];
//    QStringList deviceList;
//
//    // 检索本地机器的设备列表
//    if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &alldevs, errbuf) == -1) {
//        emit errorOccurred(QString("Error in pcap_findalldevs_ex: %1").arg(errbuf));
//        return deviceList;
//    }
//
//    // 遍历列表，收集设备名称和描述
//    for (d = alldevs; d != NULL; d = d->next) {
//        QString desc = d->description ? QString::fromUtf8(d->description) : "No description available";
//        deviceList << QString("%1 | %2").arg(QString::fromUtf8(d->name), desc);
//    }
//
//    // 释放设备列表
//    pcap_freealldevs(alldevs);
//    return deviceList;
//}
