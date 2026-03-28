# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Windows Qt/WinPcap Ethernet packet capture and send demo application. Enumerates network interfaces, captures raw Ethernet frames via WinPcap, and sends custom Ethernet packets (EtherCAT frame type 0x88A4) on a 100ms timer.

## Build Environment

- **IDE**: Visual Studio 2022 (v143 toolset), solution at `ETH/ETH.sln`
- **Qt**: 5.14.2 (msvc2017_64) via Qt VS Tools (QtMsBuild)
- **Platform**: x64 only (Debug/Release configurations)
- **Dependencies**: WinPcap (WpdPack 4.1.2) — `wpcap.lib`, `Packet.lib`, `iphlpapi.lib`, `ws2_32.lib`
- Build via Visual Studio: open `ETH/ETH.sln`, select x64 Debug/Release, build

## Architecture

- **ETH class** (`ETH/ETH/ETH.h`, `ETH/ETH/ETH.cpp`): Main QMainWindow. Handles device enumeration (`enumNetworkDevices`), device selection, packet sending (`SendPacket`, `SlotSendETH_Packet`), and MAC address resolution via Windows IPHLPAPI (matching adapter GUID from WinPcap device name).
- **CaptureThread** (defined in `ETH.h`, implemented in `ETH.cpp`): QThread subclass for non-blocking packet capture using `pcap_next_ex`. Emits `packetCaptured` signal.
- **EthDevice** (`ETH/ETH/src/EthDevice.h`, `ETH/ETH/src/EthDevice.cpp`): Stub class, mostly commented out. Not actively used.
- **UI**: Qt Designer form `ETH.ui` — QComboBox (`deviceListWidget`) for device selection, QPushButton (unused).

## Key Data Structures

- `MAC_ADDR`: 6-byte MAC address struct
- `ETH_DEVICE`: pairs a `MAC_ADDR` with a device name string
- `ETH::ETH_Device`: QVector storing all enumerated devices with their MACs

## Include Path Notes

The `.vcxproj` has hardcoded include paths (e.g., `D:\code\WpdPack_4_1_2\WpdPack\Include`). These must be updated to match the local WinPcap SDK location when building on a different machine.
