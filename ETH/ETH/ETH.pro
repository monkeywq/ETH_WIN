QT       += core gui widgets

CONFIG   += c++11
TARGET    = ETH
TEMPLATE  = app

# ========== SOEM 路径 ==========
SOEM_DIR = $$PWD/src/SOEM

INCLUDEPATH += \
    $$SOEM_DIR/soem \
    $$SOEM_DIR/osal \
    $$SOEM_DIR/osal/win32 \
    $$SOEM_DIR/oshw/win32 \
    $$SOEM_DIR/oshw/win32/wpcap/Include \
    $$PWD/src

# SOEM核心源文件
SOEM_SOURCES = \
    $$SOEM_DIR/soem/ethercatbase.c \
    $$SOEM_DIR/soem/ethercatcoe.c \
    $$SOEM_DIR/soem/ethercatconfig.c \
    $$SOEM_DIR/soem/ethercatdc.c \
    $$SOEM_DIR/soem/ethercateoe.c \
    $$SOEM_DIR/soem/ethercatfoe.c \
    $$SOEM_DIR/soem/ethercatmain.c \
    $$SOEM_DIR/soem/ethercatprint.c \
    $$SOEM_DIR/soem/ethercatsoe.c \
    $$SOEM_DIR/osal/win32/osal.c \
    $$SOEM_DIR/oshw/win32/nicdrv.c \
    $$SOEM_DIR/oshw/win32/oshw.c

# MinGW 库（SOEM自带的WinPcap 32位 .a 库）
LIBS += -L$$SOEM_DIR/oshw/win32/wpcap/Lib \
        -lwpcap \
        -lpacket \
        -liphlpapi \
        -lws2_32 \
        -lwinmm

# SOEM C文件需要定义WIN32和HAVE_REMOTE
DEFINES += WIN32 HAVE_REMOTE EC_VER1

# ========== 项目源文件 ==========
SOURCES += \
    main.cpp \
    ETH.cpp \
    src/SoemMaster.cpp \
    $$SOEM_SOURCES

HEADERS += \
    ETH.h \
    src/SoemMaster.h

FORMS += \
    ETH.ui

RESOURCES += \
    ETH.qrc
