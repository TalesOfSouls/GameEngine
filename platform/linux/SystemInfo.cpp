/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_LINUX_SYSTEM_INFO_C
#define COMS_PLATFORM_LINUX_SYSTEM_INFO_C

#include "../../stdlib/Stdlib.h"
#include "../../system/SystemInfo.h"
#include "../../architecture/CpuInfo.cpp"
#include "../../system/FileUtils.cpp"

#include <locale.h>
#include <sys/resource.h>
#include <dirent.h>

// -lX11 -lXrandr

// @todo Implement own line by line file reading

uint64 system_private_memory_usage()
{
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss * 1024;  // Convert from kilobytes to bytes
    } else {
        return 0;
    }
}

inline
uint32 system_stack_usage()
{
    static thread_local char* stack_base = NULL;

    if (!stack_base) {
        char path[64];
        pid_t tid = get_tid();
        if (tid == getpid()) {
            // main thread: use /proc/self/maps
            str_copy(path, "/proc/self/maps", sizeof("/proc/self/maps"));
        } else {
            // other thread: use task-specific maps
            sprintf_fast(path, sizeof(path), "/proc/self/task/%d/maps", (uint32) tid)
        }

        // @todo replace with own functions
        FILE *f = fopen(path, "r");

        char line[512];
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, "[stack]")) {
                unsigned long start = 0;
                if (sscanf(line, "%lx-", &start) == 2) {
                    stack_base = (uintptr_t) start;
                }

                break;
            }
        }

        fclose(f);
    }

    volatile char local_var = '0'; // current position on stack
    uint32 used = (uint32) (stack_base - (char *) &local_var);

    return used;
}

uint64 system_app_memory_usage()
{
    uint64 total_size = 0;

    FileHandle fp = file_read_handle("/proc/self/smaps");
    char line[256];
    char internal_buffer[512];
    ssize_t internal_buffer_size = 0;
    char* internal_pos = NULL;

    while (file_read_line(fp, line, sizeof(line), internal_buffer, &internal_buffer_size, &internal_pos)) {
        if (strncmp(line, "Private_Dirty:", sizeof("Private_Dirty:") - 1) == 0) {
            uint64 private_dirty;
            if (sscanf(line, "Private_Dirty: %lu kB", &private_dirty) == 1) {
                total_size += private_dirty * 1024;  // Convert from kB to bytes
            }
        }
    }

    file_close_handle(fp);

    return total_size;
}

uint16 system_language_code()
{
    const char* localeName = setlocale(LC_ALL, "");

    return (localeName[0] << 8) | localeName[1];
}

uint16 system_country_code()
{
    const char* localeName = setlocale(LC_ALL, "");

    return (localeName[3] << 8) | localeName[4];
}

void mainboard_info_get(MainboardInfo* info) {
    FileBody file;

    file.content = (byte *) info->name;
    file.size = sizeof(info->name) - 1;
    file_read("/sys/class/dmi/id/board_name", &file);
    info->name[sizeof(info->name) - 1] = '\0';

    file.content = (byte *) info->serial_number;
    file.size = sizeof(info->serial_number) - 1;
    file_read("/sys/class/dmi/id/board_serial", &file);
    info->name[sizeof(info->serial_number) - 1] = '\0';
}

int32 network_info_get(NetworkInfo* info) {
    char path[64] = "/sys/class/net/eth";

    struct stat st;
    int32 i = 0;

    FileBody file = {0};

    for (i = 0; i < 4; ++i) {
        int_to_str(i, path + sizeof("/sys/class/net/eth") - 1);

        if (stat(path, &st) != 0) {
            break;
        }

        // Read MAC address
        memcpy(path + sizeof("/sys/class/net/eth"), "/address", sizeof("/address"));

        file.content = info[i].mac;
        file.size = sizeof(info[i].mac) - 1;
        file_read(path, &file);

        // Read interface name
        memcpy(path + sizeof("/sys/class/net/eth"), "/ifindex", sizeof("/ifindex"));

        file.content = (byte *) info[i].slot;
        file.size = sizeof(info[i].slot) - 1;
        file_read(path, &file);
    }

    return i;
}

void cpu_info_get(CpuInfo* info) {
    info->features = cpu_info_features();

    cpu_info_cache(1, &info->cache[0]);
    cpu_info_cache(2, &info->cache[1]);
    cpu_info_cache(3, &info->cache[2]);
    cpu_info_cache(4, &info->cache[3]);

    FileHandle fp = file_read_handle("/proc/cpuinfo");
    char line[256];
    char internal_buffer[512];
    ssize_t internal_buffer_size = 0;
    char* internal_pos = NULL;

    while (file_read_line(fp, line, sizeof(line), internal_buffer, &internal_buffer_size, &internal_pos)) {
        if (strncmp(line, "vendor_id", sizeof("vendor_id") - 1) == 0) {
            sscanf(line, "vendor_id : %s", info->vendor);
        } else if (strncmp(line, "model", sizeof("model") - 1) == 0) {
            sscanf(line, "model : %hhd", &info->model);
        } else if (strncmp(line, "cpu MHz", sizeof("cpu MHz") - 1) == 0) {
            sscanf(line, "cpu MHz : %d", &info->mhz);
        } else if (strncmp(line, "cpu cores", sizeof("cpu cores") - 1) == 0) {
            sscanf(line, "cpu cores : %hd", &info->core_count);
        } else if (strncmp(line, "model name", sizeof("model name") - 1) == 0) {
            sscanf(line, "model name : %63[^\n]", info->brand);
        }
    }

    file_close_handle(fp);

    info->family = 0;
    info->page_size = 4096;  // Assuming standard page size of 4KB in Linux
}

void os_info_get(OSInfo* info) {
    memcpy(info->vendor, "Linux", sizeof("Linux"));
    memcpy(info->name, "Linux", sizeof("Linux"));
    info->major = 0;
    info->minor = 0;
}

void ram_info_get(RamInfo* info) {
    uint32 total_memory = 0;

    FileHandle fp = file_read_handle("/proc/meminfo");
    char line[256];
    char internal_buffer[512];
    ssize_t internal_buffer_size = 0;
    char* internal_pos = NULL;

    while (file_read_line(fp, line, sizeof(line), internal_buffer, &internal_buffer_size, &internal_pos)) {
        if (sscanf(line, "MemTotal: %u kB", &total_memory) == 1) {
            break;
        }
    }

    file_close_handle(fp);

    // Convert memory from kB to MB
    info->memory = total_memory / 1024;
}

RamChannelType ram_channel_info() {
    FILE* fp;
    char buffer[128];
    int32 ram_module_count = 0;
    int32 dual_channel_capable = 0;

    fp = popen("dmidecode -t memory | grep 'Channel'", "r");
    if (fp == NULL) {
        return RAM_CHANNEL_TYPE_FAILED;
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (strstr(buffer, "ChannelA") || strstr(buffer, "ChannelB")) {
            ++ram_module_count;
            dual_channel_capable = 1;
        } else if (strstr(buffer, "Channel")) {
            ++ram_module_count;
        }
    }
    pclose(fp);

    if (ram_module_count == 1) {
        return RAM_CHANNEL_TYPE_SINGLE_CHANNEL;
    } else if (ram_module_count == 2 && dual_channel_capable) {
        return RAM_CHANNEL_TYPE_DUAL_CHANNEL;
    } else if (ram_module_count == 2 && !dual_channel_capable) {
        return RAM_CHANNEL_TYPE_CAN_UPGRADE;
    } else {
        return RAM_CHANNEL_TYPE_FAILED;
    }
}

uint32 gpu_info_get(GpuInfo* info) {
    FILE* fp = popen("lspci | grep VGA", "r");
    if (fp == NULL) {
        return 0;
    }

    uint32 count = 0;
    char line[256];

    while (fgets(line, sizeof(line), fp)) {
        if (count >= 2) break;

        // Assuming that the first part of the line contains the name of the GPU
        char* gpu_name = strtok(line, ":");
        if (gpu_name) {
            sprintf_fast(info[count].name, sizeof(info[count].name), "%s", gpu_name);
        }

        // @todo this is Wrong
        info[count].vram = 2048;

        ++count;
    }

    pclose(fp);

    return count;
}

uint32 display_info_get(DisplayInfo* info) {
    FILE* fp = popen("xrandr --current 2>/dev/null", "r");
    if (fp == NULL) {
        return 0;
    }

    char line[256];
    uint32 count = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (str_find(line, "connected") >= 0) {
            // Example: "HDMI-1 connected 1920x1080+0+0 60.00*+"
            char name[64];
            uint32 width, height, hz;
            if (sscanf(line, "%s connected %dx%d+%*d+%*d %d", name, &width, &height, &hz) == 4) {
                str_copy(info[count].name, name);
                info[count].width = width;
                info[count].height = height;
                info[count].hz = hz;
                info[count].is_primary = str_find(line, "primary") >= 0;
                ++count;
            }
        }
    }

    pclose(fp);

    return count;
}

static inline char hardware_id_lower(char value) NO_EXCEPT
{
    return (value >= 'A' && value <= 'Z') ? (char) (value + ('a' - 'A')) : value;
}

static inline bool hardware_id_equal_ci(const char* lhs, const char* rhs) NO_EXCEPT
{
    if (!lhs || !rhs) {
        return false;
    }

    while (*lhs && *rhs) {
        if (hardware_id_lower(*lhs) != hardware_id_lower(*rhs)) {
            return false;
        }

        ++lhs;
        ++rhs;
    }

    return *lhs == *rhs;
}

static inline bool hardware_id_contains_ci(const char* value, const char* needle) NO_EXCEPT
{
    if (!value || !needle || !*needle) {
        return false;
    }

    const size_t needle_length = strlen(needle);
    for (const char* cursor = value; *cursor; ++cursor) {
        size_t i = 0;
        while (i < needle_length && cursor[i]
            && hardware_id_lower(cursor[i]) == hardware_id_lower(needle[i])
        ) {
            ++i;
        }

        if (i == needle_length) {
            return true;
        }
    }

    return false;
}

static inline void hardware_id_trim(char* const value) NO_EXCEPT
{
    if (!value) {
        return;
    }

    char* start = value;
    while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') {
        ++start;
    }

    if (start != value) {
        char* dst = value;
        while (*start) {
            *dst++ = *start++;
        }

        *dst = '\0';
    }

    size_t length = strlen(value);
    while (length && (value[length - 1] == ' ' || value[length - 1] == '\t'
        || value[length - 1] == '\r' || value[length - 1] == '\n')
    ) {
        value[--length] = '\0';
    }
}

static inline void hardware_id_upper(char* const value) NO_EXCEPT
{
    if (!value) {
        return;
    }

    for (; *value; ++value) {
        if (*value >= 'a' && *value <= 'z') {
            *value = (char) (*value - ('a' - 'A'));
        }
    }
}

static inline bool hardware_id_ignored(const char* const value) NO_EXCEPT
{
    static const char* const ignored[] = {
        "none",
        "n/a",
        "na",
        "unknown",
        "default string",
        "to be filled by o.e.m.",
        "to be filled by oem",
        "system serial number",
        "system manufacturer",
        "system product name",
        "not specified",
        "not available",
        "0",
        "00000000",
        "ffffffff",
        "ffffffffffffffff",
        "00000000-0000-0000-0000-000000000000",
        "ffffffff-ffff-ffff-ffff-ffffffffffff",
        "03000200-0400-0500-0006-000700080009",
    };

    for (size_t i = 0; i < ARRAY_COUNT(ignored); ++i) {
        if (hardware_id_equal_ci(value, ignored[i])) {
            return true;
        }
    }

    return false;
}

static inline bool hardware_id_read_file(
    const char* const path,
    char* const out,
    size_t out_size
) NO_EXCEPT
{
    if (!path || !out || !out_size) {
        return false;
    }

    memset(out, 0, out_size);

    FileBody file = {};
    file.content = (byte *) out;
    file.size = out_size - 1;

    file_read(path, &file);
    out[out_size - 1] = '\0';
    hardware_id_trim(out);

    return out[0] != '\0';
}

static inline void hardware_id_read_dmi(
    const char* const name,
    char* const out,
    size_t out_size,
    bool uppercase
) NO_EXCEPT
{
    char path[PATH_MAX_LENGTH];
    sprintf_fast(path, (int32) sizeof(path), "/sys/class/dmi/id/%s", name);

    if (!hardware_id_read_file(path, out, out_size)) {
        return;
    }

    if (uppercase) {
        hardware_id_upper(out);
    }

    if (hardware_id_ignored(out)) {
        out[0] = '\0';
    }
}

static inline void hardware_id_hex4(char* const out, uint32 value) NO_EXCEPT
{
    static const char HEX[] = "0123456789ABCDEF";

    for (int32 i = 3; i >= 0; --i) {
        out[i] = HEX[value & 0xF];
        value >>= 4;
    }

    out[4] = '\0';
}

static inline void hardware_id_gpu_collect(HardwareId* const id) NO_EXCEPT
{
    DIR* const directory = opendir("/sys/bus/pci/devices");
    if (!directory) {
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(directory))) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        char path[PATH_MAX_LENGTH];
        sprintf_fast(
            path,
            (int32) sizeof(path),
            "/sys/bus/pci/devices/%s/class",
            entry->d_name
        );

        char pci_class[32];
        if (!hardware_id_read_file(path, pci_class, sizeof(pci_class))) {
            continue;
        }

        // 0x03xxxx = display controller (VGA, XGA, 3D, other)
        if (strncmp(pci_class, "0x03", 4) != 0) {
            continue;
        }

        sprintf_fast(
            path,
            (int32) sizeof(path),
            "/sys/bus/pci/devices/%s/uevent",
            entry->d_name
        );

        char uevent[2048];
        if (!hardware_id_read_file(path, uevent, sizeof(uevent))) {
            continue;
        }

        const char* const pci_id = strstr(uevent, "PCI_ID=");
        const char* const subsystem_id = strstr(uevent, "PCI_SUBSYS_ID=");
        if (!pci_id || !subsystem_id) {
            continue;
        }

        uint32 vendor = 0;
        uint32 device = 0;
        uint32 subsystem_vendor = 0;
        uint32 subsystem_device = 0;

        if (sscanf(pci_id, "PCI_ID=%4x:%4x", &vendor, &device) != 2
            || sscanf(subsystem_id, "PCI_SUBSYS_ID=%4x:%4x", &subsystem_vendor, &subsystem_device) != 2
        ) {
            continue;
        }

        char vendor_hex[5];
        char device_hex[5];
        char subsystem_vendor_hex[5];
        char subsystem_device_hex[5];

        hardware_id_hex4(vendor_hex, vendor);
        hardware_id_hex4(device_hex, device);
        hardware_id_hex4(subsystem_vendor_hex, subsystem_vendor);
        hardware_id_hex4(subsystem_device_hex, subsystem_device);

        char gpu_id[128];
        sprintf_fast(
            gpu_id,
            (int32) sizeof(gpu_id),
            "VEN_%s&DEV_%s&SUBSYS_%s%s",
            vendor_hex,
            device_hex,
            subsystem_device_hex,
            subsystem_vendor_hex
        );

        hardware_id_gpu_add(id, gpu_id);
    }

    closedir(directory);

    hardware_id_gpu_sort(id);
}

void hardware_id_get(HardwareId* const id, const SystemInfo* const info) NO_EXCEPT
{
    (void) info;

    if (!id) {
        return;
    }

    memset(id, 0, sizeof(*id));

    hardware_id_read_dmi("product_uuid", id->system_uuid, sizeof(id->system_uuid), true);
    hardware_id_read_dmi("product_serial", id->system_serial, sizeof(id->system_serial), true);
    hardware_id_read_dmi("sys_vendor", id->system_vendor, sizeof(id->system_vendor), false);
    hardware_id_read_dmi("product_name", id->system_product, sizeof(id->system_product), false);
    hardware_id_read_dmi("board_serial", id->board_serial, sizeof(id->board_serial), true);

    hardware_id_cpu_fill(id);
    hardware_id_gpu_collect(id);
}

static inline bool hardware_id_known_vm_string(const char* const value) NO_EXCEPT
{
    static const char* const markers[] = {
        "vmware",
        "virtualbox",
        "vbox",
        "qemu",
        "xen",
        "parallels",
        "bhyve",
        "bochs",
        "innotek",
        "microsoft hv",
        "hyper-v",
        "virtual machine",
        "kvm",
        "openstack",
        "amazon ec2",
        "google compute",
        "digitalocean",
        "alibaba",
    };

    for (size_t i = 0; i < ARRAY_COUNT(markers); ++i) {
        if (hardware_id_contains_ci(value, markers[i])) {
            return true;
        }
    }

    return false;
}

bool system_is_virtual_machine(const SystemInfo* const info) NO_EXCEPT
{
    // Xen, Hyper-V and some KVM setups expose the hypervisor type here
    if (file_exists("/sys/hypervisor/type")) {
        return true;
    }

    char cpuinfo[16384] = {};
    FileBody file = {};
    file.content = (byte *) cpuinfo;
    file.size = sizeof(cpuinfo) - 1;

    file_read("/proc/cpuinfo", &file);
    cpuinfo[sizeof(cpuinfo) - 1] = '\0';

    if (hardware_id_contains_ci(cpuinfo, "hypervisor")) {
        return true;
    }

    if (info) {
        if (hardware_id_known_vm_string(info->hardware.system_vendor)
            || hardware_id_known_vm_string(info->hardware.system_product)
        ) {
            return true;
        }
    }

    return false;
}

#endif