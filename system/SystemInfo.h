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

#include "../stdlib/Types.h"
#include "../architecture/CpuInfo.h"

// @todo add vendor name
struct MainboardInfo {
    char name[64];
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

struct GpuInfo {
    char name[64];
    uint32 vram;
};

struct DisplayInfo {
    char name[64];
    int32 width;
    int32 height;
    int32 hz;
    bool is_primary;
};

struct SystemInfo {
    OSInfo os;
    MainboardInfo mainboard;

    NetworkInfo network[4];
    int32 network_count;

    CpuInfo cpu;
    RamInfo ram;

    GpuInfo gpu[3];
    int32 gpu_count;

    DisplayInfo display[6];
    int32 display_count;

    int32 language;
};

enum RamChannelType {
    RAM_CHANNEL_TYPE_FAILED,
    RAM_CHANNEL_TYPE_SINGLE_CHANNEL,
    RAM_CHANNEL_TYPE_CAN_UPGRADE,
    RAM_CHANNEL_TYPE_DUAL_CHANNEL,
};

enum DriveType {
    DRIVE_TYPE_UNKNOWN,
    DRIVE_TYPE_NVME,
    DRIVE_TYPE_SSD,
    DRIVE_TYPE_HDD,
};

#endif