/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_SYSTEM_INFO_C
#define COMS_SYSTEM_INFO_C

#include "../stdlib/Stdlib.h"
#include "SystemInfo.h"
#include "../hash/Sha256.h"
#include "../utils/StringUtils.h"

#if _WIN32
    #include "../platform/win32/SystemInfo.cpp"
#elif __linux__
    #include "../platform/linux/SystemInfo.cpp"
#endif

static inline void hardware_id_hex32(char* const out, uint32 value) NO_EXCEPT
{
    static const char HEX[] = "0123456789ABCDEF";

    for (int32 i = 7; i >= 0; --i) {
        out[i] = HEX[value & 0xF];
        value >>= 4;
    }

    out[8] = '\0';
}

/**
 * Trims and uppercases a hardware identifier
 *
 * The same DMI string can be reported with different case by Windows and Linux,
 * so the hash input must be normalized.
 */
static inline void hardware_id_normalize(char* const value) NO_EXCEPT
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

    for (size_t i = 0; i < length; ++i) {
        if (value[i] >= 'a' && value[i] <= 'z') {
            value[i] = (char) (value[i] - ('a' - 'A'));
        }
    }
}

void hardware_id_cpu_fill(HardwareId* const id) NO_EXCEPT
{
    if (!id) {
        return;
    }

#if defined(__aarch64__) || defined(_M_ARM64)
    // CPUID isn't available on ARM; the system UUID remains the primary id.
    id->cpu_id[0] = '\0';
#else
    uint32 regs[4];

    compiler_cpuid(regs, 0);

    char vendor[13];
    memcpy(vendor, &regs[1], 4);
    memcpy(vendor + 4, &regs[3], 4);
    memcpy(vendor + 8, &regs[2], 4);
    vendor[12] = '\0';

    compiler_cpuid(regs, 1);

    const uint32 eax = regs[0];
    const uint32 edx = regs[3];

    const uint32 stepping = eax & 0xF;
    uint32 model = (eax >> 4) & 0xF;
    uint32 family = (eax >> 8) & 0xF;

    if (family == 0xF) {
        family += (eax >> 20) & 0xFF;
    }

    if (family == 0x6 || family == 0xF) {
        model += ((eax >> 16) & 0xF) << 4;
    }

    char eax_hex[9];
    char edx_hex[9];

    hardware_id_hex32(eax_hex, eax);
    hardware_id_hex32(edx_hex, edx);

    sprintf_fast(
        id->cpu_id,
        (int32) sizeof(id->cpu_id),
        "%s|%d|%d|%d|%s|%s",
        vendor,
        (int32) family,
        (int32) model,
        (int32) stepping,
        eax_hex,
        edx_hex
    );
#endif
}

void hardware_id_gpu_add(HardwareId* const id, const char* const value) NO_EXCEPT
{
    if (!id || !value) {
        return;
    }

    char normalized[128];
    const size_t length = strlen(value) < sizeof(normalized) - 1
        ? strlen(value)
        : sizeof(normalized) - 1;

    for (size_t i = 0; i < length; ++i) {
        normalized[i] = value[i];
    }

    normalized[length] = '\0';
    hardware_id_normalize(normalized);

    if (!normalized[0] || id->gpu_id_count >= ARRAY_COUNT(id->gpu_id)) {
        return;
    }

    for (uint8 i = 0; i < id->gpu_id_count; ++i) {
        if (strcmp(id->gpu_id[i], normalized) == 0) {
            return;
        }
    }

    memcpy(id->gpu_id[id->gpu_id_count], normalized, strlen(normalized) + 1);
    ++id->gpu_id_count;
}

void hardware_id_gpu_sort(HardwareId* const id) NO_EXCEPT
{
    if (!id) {
        return;
    }

    for (uint8 i = 0; i < id->gpu_id_count; ++i) {
        for (uint8 j = (uint8) (i + 1); j < id->gpu_id_count; ++j) {
            if (strcmp(id->gpu_id[i], id->gpu_id[j]) > 0) {
                char temp[128];
                memcpy(temp, id->gpu_id[i], sizeof(temp));
                memcpy(id->gpu_id[i], id->gpu_id[j], sizeof(temp));
                memcpy(id->gpu_id[j], temp, sizeof(temp));
            }
        }
    }
}

static inline int32 hardware_id_append(
    char* const buffer,
    int32 capacity,
    int32 offset,
    const char* const label,
    const char* const value
) NO_EXCEPT
{
    if (!value || !value[0] || offset >= capacity - 1) {
        return offset;
    }

    const int32 written = sprintf_fast(buffer + offset, capacity - offset, "%s=%s;", label, value);
    if (written <= 0) {
        return offset;
    }

    offset += written;
    return offset < capacity ? offset : capacity - 1;
}

bool system_hardware_uuid(const SystemInfo* const info, char* const out, size_t out_size) NO_EXCEPT
{
    if (!info || !out || out_size < 37) {
        return false;
    }

    out[0] = '\0';

    const HardwareId* const id = &info->hardware;

    // The system UUID is the only value that is both machine-unique and exposed
    // identically by Windows and Linux. Without it the result would not match
    // across operating systems.
    if (!id->system_uuid[0]) {
        return false;
    }

    char payload[1024];
    int32 length = sprintf_fast(payload, (int32) sizeof(payload), "TOS-HWID-2|");

    length = hardware_id_append(payload, (int32) sizeof(payload), length, "sys_uuid", id->system_uuid);
    length = hardware_id_append(payload, (int32) sizeof(payload), length, "cpu", id->cpu_id);
    length = hardware_id_append(payload, (int32) sizeof(payload), length, "gpu0", id->gpu_id[0]);
    length = hardware_id_append(payload, (int32) sizeof(payload), length, "gpu1", id->gpu_id[1]);
    length = hardware_id_append(payload, (int32) sizeof(payload), length, "gpu2", id->gpu_id[2]);

    if (length <= 0) {
        return false;
    }

    byte digest[SHA256_DIGEST_SIZE];
    sha256_hash((const byte *) payload, (size_t) length, digest);

    // RFC 9562 version 8 (custom, SHA-256 based) + RFC 4122 variant bits
    digest[6] = (byte) ((digest[6] & 0x0F) | 0x80);
    digest[8] = (byte) ((digest[8] & 0x3F) | 0x80);

    char hex[33];
    bytes_to_hex(digest, 16, hex);
    hex[32] = '\0';

    memcpy(out, hex, 8);
    out[8] = '-';
    memcpy(out + 9, hex + 8, 4);
    out[13] = '-';
    memcpy(out + 14, hex + 12, 4);
    out[18] = '-';
    memcpy(out + 19, hex + 16, 4);
    out[23] = '-';
    memcpy(out + 24, hex + 20, 12);
    out[36] = '\0';

    return true;
}

void system_info_render(char* buf, const SystemInfo* const info) NO_EXCEPT
{
    sprintf_fast(
        buf,
        4096,
        "OS:\n"
        "==============\n"
        "Vendor: %s\n" "Name: %s\n" "Major: %d\n" "Minor: %d\n"
        "\n"
        "Mainboard:\n"
        "==============\n"
        "Name: %s\n" "SN: %s\n"
        "\n"
        "Network:\n"
        "==============\n"
        "Slot: %s\n" "MAC: %h24\n"
        "\n"
        "Slot: %s\n" "MAC: %h24\n"
        "\n"
        "Slot: %s\n" "MAC: %h24\n"
        "\n"
        "Slot: %s\n" "MAC: %h24\n"
        "\n"
        "CPU:\n"
        "==============\n"
        "Hardware\n" "Vendor: %s\n" "Brand: %s\n" "Model: %d\n" "Family: %d\n" "Mhz: %d\n" "Core Count: %d\n" "Page Size: %d\n"
        "\n"
        "Cache:\n"
        "L1: Size %d Line %d\n"
        "L2: Size %d Line %d\n"
        "L3: Size %d Line %d\n"
        "L4: Size %d Line %d\n"
        "\n"
        "Features: %l\n"
        "\n"
        "Drive:\n"
        "==============\n"
        "Total %l Free %l\n"
        "\n"
        "GPU:\n"
        "==============\n"
        "Name: %s\n" "VRAM: %d\n"
        "Name: %s\n" "VRAM: %d\n"
        "Name: %s\n" "VRAM: %d\n"
        "\n"
        "Display:\n"
        "==============\n"
        "Name: %s " "Width: %d " "Height: %d " "Hz: %d " "Primary: %d\n"
        "Name: %s " "Width: %d " "Height: %d " "Hz: %d " "Primary: %d\n"
        "Name: %s " "Width: %d " "Height: %d " "Hz: %d " "Primary: %d\n"
        "Name: %s " "Width: %d " "Height: %d " "Hz: %d " "Primary: %d\n"
        "Name: %s " "Width: %d " "Height: %d " "Hz: %d " "Primary: %d\n"
        "Name: %s " "Width: %d " "Height: %d " "Hz: %d " "Primary: %d\n"
        "\n"
        "RAM:\n"
        "==============\n"
        "Memory: %d MB",
        info->os.vendor, info->os.name, info->os.major, info->os.minor,
        info->mainboard.name, info->mainboard.serial_number,
        info->network[0].slot, info->network[0].mac,
        info->network_count < 2 ? "" : info->network[1].slot, info->network_count < 2 ? 0 : info->network[1].mac,
        info->network_count < 3 ? "" : info->network[2].slot, info->network_count < 3 ? 0 : info->network[2].mac,
        info->network_count < 4 ? "" : info->network[3].slot, info->network_count < 4 ? 0 : info->network[3].mac,
        info->cpu.vendor, info->cpu.brand, info->cpu.model, info->cpu.family, info->cpu.mhz, info->cpu.core_count, info->cpu.page_size,
        info->cpu.cache[0].size, (uint32) info->cpu.cache[0].line_size,
        info->cpu.cache[1].size, (uint32) info->cpu.cache[1].line_size,
        info->cpu.cache[2].size, (uint32) info->cpu.cache[2].line_size,
        info->cpu.cache[3].size, (uint32) info->cpu.cache[3].line_size,
        info->cpu.features,
        info->drive.total, info->drive.free,
        info->gpu[0].name, info->gpu[0].vram,
        info->gpu_count < 2 ? "" : info->gpu[1].name, info->gpu_count < 2 ? 0 : info->gpu[1].vram,
        info->gpu_count < 3 ? "" : info->gpu[2].name, info->gpu_count < 3 ? 0 : info->gpu[2].vram,
        info->display[0].name, info->display[0].width, info->display[0].height, info->display[0].hz, info->display[0].is_primary,
        info->display_count < 2 ? "" : info->display[1].name, info->display_count < 2 ? 0 : info->display[1].width, info->display_count < 2 ? 0 : info->display[1].height, info->display_count < 2 ? 0 : info->display[1].hz, info->display_count < 2 ? 0 : info->display[1].is_primary,
        info->display_count < 3 ? "" : info->display[2].name, info->display_count < 3 ? 0 : info->display[2].width, info->display_count < 3 ? 0 : info->display[2].height, info->display_count < 3 ? 0 : info->display[2].hz, info->display_count < 3 ? 0 : info->display[2].is_primary,
        info->display_count < 4 ? "" : info->display[3].name, info->display_count < 4 ? 0 : info->display[3].width, info->display_count < 4 ? 0 : info->display[3].height, info->display_count < 4 ? 0 : info->display[3].hz, info->display_count < 4 ? 0 : info->display[3].is_primary,
        info->display_count < 5 ? "" : info->display[4].name, info->display_count < 5 ? 0 : info->display[4].width, info->display_count < 5 ? 0 : info->display[4].height, info->display_count < 5 ? 0 : info->display[4].hz, info->display_count < 5 ? 0 : info->display[4].is_primary,
        info->display_count < 6 ? "" : info->display[5].name, info->display_count < 6 ? 0 : info->display[5].width, info->display_count < 6 ? 0 : info->display[5].height, info->display_count < 6 ? 0 : info->display[5].hz, info->display_count < 6 ? 0 : info->display[5].is_primary,
        info->ram.memory
    );
}

void system_info_get(SystemInfo* const info) NO_EXCEPT
{
    os_info_get(&info->os);
    mainboard_info_get(&info->mainboard);
    info->network_count = (uint8) network_info_get(info->network, ARRAY_COUNT(info->network));
    cpu_info_get(&info->cpu);
    ram_info_get(&info->ram);
    info->gpu_count = (uint8) gpu_info_get(info->gpu, ARRAY_COUNT(info->gpu));
    info->display_count = (uint8) display_info_get(info->display, ARRAY_COUNT(info->display));
    hardware_id_get(&info->hardware, info);
    system_hardware_uuid(info, info->hardware.uuid, sizeof(info->hardware.uuid));
    info->language = system_language_code();
    info->country = system_country_code();

    wchar_t path[PATH_MAX_LENGTH];
    self_directory_path(path);

    // @bug only works on windows
    wchar_t* temp = path;
    while (*temp != L':' && *temp) {
        ++temp;
    }

    if (*temp == L':') {
        temp[1] = L'\\';
        temp[2] = L'\\';
        temp[3] = L'\0';
        system_drive_space(path, &info->drive.total, &info->drive.free);
    }
}

#endif