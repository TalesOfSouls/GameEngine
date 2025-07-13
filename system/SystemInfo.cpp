/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_SYSTEM_INFO_C
#define COMS_SYSTEM_INFO_C

#include <inttypes.h>
#if _WIN32
    #include "../platform/win32/SystemInfo.cpp"
#elif __linux__
    #include "../platform/linux/SystemInfo.cpp"
#endif

void system_info_render(char* buf, const SystemInfo* info) {
    snprintf(
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
        "Slot: %s\n" "MAC: %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n"
        "\n"
        "Slot: %s\n" "MAC: %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n"
        "\n"
        "Slot: %s\n" "MAC: %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n"
        "\n"
        "Slot: %s\n" "MAC: %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n"
        "\n"
        "CPU:\n"
        "==============\n"
        "Hardware\n" "Vendor: %s\n" "Brand: %s\n" "Model: %d\n" "Family: %d\n" "Mhz: %d\n" "Core Count: %d\n" "Page Size: %u\n"
        "\n"
        "Cache:\n"
        "L1: Size %u Line %u\n"
        "L2: Size %u Line %u\n"
        "L3: Size %u Line %u\n"
        "L4: Size %u Line %u\n"
        "\n"
        "Features: %" PRId64 "\n"
        "\n"
        "GPU:\n"
        "==============\n"
        "Name: %s\n" "VRAM: %u\n"
        "Name: %s\n" "VRAM: %u\n"
        "Name: %s\n" "VRAM: %u\n"
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
        "Memory: %u MB",
        info->os.vendor, info->os.name, info->os.major, info->os.minor,
        info->mainboard.name, info->mainboard.serial_number,
        info->network[0].slot, info->network[0].mac[0], info->network[0].mac[1], info->network[0].mac[2], info->network[0].mac[3], info->network[0].mac[4], info->network[0].mac[5], info->network[0].mac[6], info->network[0].mac[7],
        info->network_count < 2 ? "" : info->network[1].slot, info->network_count < 2 ? 0 : info->network[1].mac[0], info->network_count < 2 ? 0 : info->network[1].mac[1], info->network_count < 2 ? 0 : info->network[1].mac[2], info->network_count < 2 ? 0 : info->network[1].mac[3], info->network_count < 2 ? 0 : info->network[1].mac[4], info->network_count < 2 ? 0 : info->network[1].mac[5], info->network_count < 2 ? 0 : info->network[1].mac[6], info->network_count < 2 ? 0 : info->network[1].mac[7],
        info->network_count < 3 ? "" : info->network[2].slot, info->network_count < 3 ? 0 : info->network[2].mac[0], info->network_count < 3 ? 0 : info->network[2].mac[1], info->network_count < 3 ? 0 : info->network[2].mac[2], info->network_count < 3 ? 0 : info->network[2].mac[3], info->network_count < 3 ? 0 : info->network[2].mac[4], info->network_count < 3 ? 0 : info->network[2].mac[5], info->network_count < 3 ? 0 : info->network[2].mac[6], info->network_count < 3 ? 0 : info->network[2].mac[7],
        info->network_count < 4 ? "" : info->network[3].slot, info->network_count < 4 ? 0 : info->network[3].mac[0], info->network_count < 4 ? 0 : info->network[3].mac[1], info->network_count < 4 ? 0 : info->network[3].mac[2], info->network_count < 4 ? 0 : info->network[3].mac[3], info->network_count < 4 ? 0 : info->network[3].mac[4], info->network_count < 4 ? 0 : info->network[3].mac[5], info->network_count < 4 ? 0 : info->network[3].mac[6], info->network_count < 4 ? 0 : info->network[3].mac[7],
        info->cpu.vendor, info->cpu.brand, info->cpu.model, info->cpu.family, info->cpu.mhz, info->cpu.core_count, info->cpu.page_size,
        info->cpu.cache[0].size, (uint32) info->cpu.cache[0].line_size,
        info->cpu.cache[1].size, (uint32) info->cpu.cache[1].line_size,
        info->cpu.cache[2].size, (uint32) info->cpu.cache[2].line_size,
        info->cpu.cache[3].size, (uint32) info->cpu.cache[3].line_size,
        info->cpu.features,
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

void system_info_get(SystemInfo* info)
{
    os_info_get(&info->os);
    mainboard_info_get(&info->mainboard);
    info->network_count = network_info_get(info->network);
    cpu_info_get(&info->cpu);
    ram_info_get(&info->ram);
    info->gpu_count = gpu_info_get(info->gpu);
    info->display_count = display_info_get(info->display);
    info->language = system_language_code();
}

#endif