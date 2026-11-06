/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_SYSTEM_INFO_H
#define COMS_SYSTEM_INFO_H

#include "../stdlib/Stdlib.h"
#include "../architecture/CpuInfo.h"
#include "../gpuapi/GpuInfo.h"

struct MainboardInfo {
    char vendor[32]; // @todo implement mainboard vendor string
    char name[32];
    char serial_number[64];
};

/**
 * Hardware identifiers used for anti-cheat hardware bans
 *
 * The combined uuid is intentionally based only on values that Windows and Linux
 * can read from the same underlying hardware:
 *      system_uuid     SMBIOS type 1 machine UUID
 *      cpu_id          CPUID leaf 1 signature (vendor/family/model/stepping/EAX/EDX)
 *      gpu_id          canonical PCI ids: VEN_<vendor>&DEV_<device>&SUBSYS_<subsystem>
 *
 * The other fields are evidence for a ban review but are not part of the combined
 * hash because their availability/formatting differs between operating systems:
 *      system_serial   SMBIOS type 1 serial number
 *      system_vendor   SMBIOS type 1 manufacturer
 *      system_product  SMBIOS type 1 product name
 *      board_serial    SMBIOS type 2 baseboard serial number
 *      bios_serial     SMBIOS type 0 BIOS serial number (not exposed by Linux sysfs)
 *
 * WARNING: These values can be spoofed and may be duplicated by VMs or OEMs.
 * They are one signal for a ban decision, not proof of identity. Treat them as
 * personal data where required by law.
 */
struct HardwareId {
    char system_uuid[37];
    char system_serial[64];
    char system_vendor[64];
    char system_product[64];
    char board_serial[64];
    char bios_serial[64];
    char cpu_id[192];
    char gpu_id[3][128];
    char uuid[37];
    uint8 gpu_id_count;
};

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
    // Memory in megabytes
    uint32 memory;
};

struct DisplayInfo {
    char name[48];
    int32 width;
    int32 height;
    int32 hz;
    bool is_primary;
};

struct DriveInfo {
    size_t total;
    size_t free;
};

struct SystemInfo {
    uint8 gpu_count;
    uint8 display_count;
    uint8 network_count;

    uint16 language;
    uint16 country;

    OSInfo os;
    MainboardInfo mainboard;
    HardwareId hardware;

    NetworkInfo network[4];

    CpuInfo cpu;
    RamInfo ram;
    DriveInfo drive;

    GpuInfo gpu[3];

    DisplayInfo display[6];
};

/**
 * Collects the platform hardware identifiers for anti-cheat bans
 *
 * @param HardwareId* id Hardware identifiers to fill
 * @param const SystemInfo* info System information containing CPU/GPU data
 *
 * @return void
 */
void hardware_id_get(HardwareId* id, const SystemInfo* info) NO_EXCEPT;

/**
 * Fills the canonical CPUID-based cpu_id (identical on Windows and Linux)
 *
 * @param HardwareId* id Hardware identifiers to fill
 *
 * @return void
 */
void hardware_id_cpu_fill(HardwareId* id) NO_EXCEPT;

/**
 * Adds a canonical VEN_/DEV_/SUBSYS_ GPU id (deduplicated)
 *
 * @param HardwareId* id Hardware identifiers to fill
 * @param const char* value Canonical GPU PCI id
 *
 * @return void
 */
void hardware_id_gpu_add(HardwareId* id, const char* value) NO_EXCEPT;

/**
 * Sorts the collected GPU ids so the hash does not depend on enumeration order
 *
 * @param HardwareId* id Hardware identifiers to sort
 *
 * @return void
 */
void hardware_id_gpu_sort(HardwareId* id) NO_EXCEPT;

/**
 * Hashes the cross-platform hardware identifiers into a UUID-shaped ban id
 *
 * @param const SystemInfo* info System information containing the hardware identifiers
 * @param char* out Output buffer (at least 37 bytes)
 * @param size_t out_size Output buffer size
 *
 * @return bool false if no system UUID is available
 */
bool system_hardware_uuid(const SystemInfo* info, char* out, size_t out_size) NO_EXCEPT;

/**
 * Checks if the current machine looks like a virtual machine
 *
 * @param const SystemInfo* info System information with the DMI strings, may be NULL
 *
 * @return bool true if a hypervisor was detected
 */
bool system_is_virtual_machine(const SystemInfo* info) NO_EXCEPT;

enum RamChannelType : byte {
    RAM_CHANNEL_TYPE_FAILED,
    RAM_CHANNEL_TYPE_SINGLE_CHANNEL,

    // Multiple modules but all of them in the same channel (another slot would enable dual channel)
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