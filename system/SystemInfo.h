/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_SYSTEM_INFO_H
#define COMS_SYSTEM_INFO_H

#include "../stdlib/Stdlib.h"
#include "../architecture/CpuInfo.h"
#include "../gpuapi/GpuInfo.h"

// @todo Should we exchange some of the uints with int32? We often don't need uint

struct MainboardInfo {
    char vendor[32]; // @todo implement
    char name[32];
    char serial_number[64];
};

// @todo add ipv6
struct NetworkInfo {
    char slot[64];
    byte mac[24];
};

struct OSInfo {
    char vendor[16];
    char name[64];
    int32 major;
    int32 minor;
};

struct RamInfo {
    uint32 memory;
};

struct DisplayInfo {
    char name[48];
    int32 width;
    int32 height;
    int32 hz;
    bool is_primary;
};

struct SystemInfo {
    uint8 gpu_count;
    uint8 display_count;
    uint8 network_count;

    uint16 language;
    uint16 country;

    OSInfo os;
    MainboardInfo mainboard;

    NetworkInfo network[4];

    CpuInfo cpu;
    RamInfo ram;

    GpuInfo gpu[3];

    DisplayInfo display[6];
};

enum RamChannelType : byte {
    RAM_CHANNEL_TYPE_FAILED,
    RAM_CHANNEL_TYPE_SINGLE_CHANNEL,
    RAM_CHANNEL_TYPE_CAN_UPGRADE,
    RAM_CHANNEL_TYPE_DUAL_CHANNEL,
};

enum DriveType : byte {
    DRIVE_TYPE_UNKNOWN,
    DRIVE_TYPE_NVME,
    DRIVE_TYPE_SSD,
    DRIVE_TYPE_HDD,
};

#endif