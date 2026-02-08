/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_LINUX_SYSTEM_INFO_C
#define COMS_PLATFORM_LINUX_SYSTEM_INFO_C

#include "../../stdlib/Stdlib.h"
#include "../../system/SystemInfo.h"
#include "../../architecture/CpuInfo.cpp"
#include "../../system/FileUtils.cpp"

#include <locale.h>
#include <sys/resource.h>

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

#endif