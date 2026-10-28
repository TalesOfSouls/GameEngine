/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_SYSTEM_INFO_C
#define COMS_PLATFORM_WIN32_SYSTEM_INFO_C

#include "../../stdlib/Stdlib.h"
#include "../../utils/StringUtils.h"
#include "../../utils/Utils.h"
#include "../../utils/TimeUtils.h"
#include "../../system/SystemInfo.h"
#include "../../architecture/CpuInfo.cpp"
#include "../../hash/Sha1.h"

#include <psapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wbemidl.h>
#include <winnls.h>
#include <wingdi.h>
#include <oleauto.h>

#include "libs/ole32.h"
#include "libs/setupapi.h"
#include "libs/iphlpapi.h"
#include "libs/wbemuuid.h"
#include "libs/Advapi32.h"
#include "libs/hid.h"

#include <intrin.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "ntdll.lib")

// @question can we remove this and make it manual loaded?
#pragma comment(lib, "oleaut32.lib")

f64 system_cpu_benchmark(int iterations)
{
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);

    byte buffer[1024];
    for (size_t i = 0; i < sizeof(buffer); ++i) {
        buffer[i] = (byte)i;
    }

    QueryPerformanceCounter(&start);

    byte result[20];
    for (int i = 0; i < iterations; ++i) {
        sha1_hash(buffer, ARRAY_COUNT(buffer), result);
        memcpy(buffer, result, sizeof(result));
    }

    QueryPerformanceCounter(&end);

    // Checking the hash output keeps the whole chain observable, otherwise the loop is dead code
    return freq.QuadPart > 0 && !is_empty(result, sizeof(result))
        ? (f64) (end.QuadPart - start.QuadPart) / freq.QuadPart
        : 0.0;
}

FORCE_INLINE
uint64 system_private_memory_usage() NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_SYS_CALL);

    // The debug overlay asks for this every frame but the value only changes slowly,
    // so one query per 250 ms is plenty and saves the syscall on all other frames
    static thread_local uint64 cached_usage = 0;
    static thread_local LONGLONG next_query = 0;
    static thread_local LONGLONG query_interval = 0;

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    if (now.QuadPart >= next_query) {
        if (!query_interval) {
            LARGE_INTEGER frequency;
            QueryPerformanceFrequency(&frequency);
            query_interval = frequency.QuadPart / 4;
        }

        PROCESS_MEMORY_COUNTERS_EX pmc = {};
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *) &pmc, sizeof(pmc))) {
            cached_usage = pmc.PrivateUsage;
        }

        next_query = now.QuadPart + query_interval;
    }

    return cached_usage;
}

FORCE_INLINE
bool system_drive_space(
    const wchar_t* drive_path,
    size_t* total_bytes,
    size_t* free_bytes
) NO_EXCEPT
{
    ULARGE_INTEGER user_free;
    ULARGE_INTEGER total_space;
    ULARGE_INTEGER total_free;

    BOOL result = GetDiskFreeSpaceExW(
        drive_path,
        &user_free,
        &total_space,
        &total_free
    );

    if (!result) {
        return false;
    }

    if (total_bytes) {
        *total_bytes = total_space.QuadPart;
    }

    if (free_bytes) {
        *free_bytes = total_free.QuadPart;
    }

    return true;
}

#include <winternl.h>
void system_cpu_usage(f32* const __restrict usage, int32 core_count, RingMemory* const __restrict ring) {
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION* p1 = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *) memory_get(
        ring,
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * core_count,
        sizeof(size_t)
    );
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION* p2 = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *) memory_get(
        ring,
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * core_count,
        sizeof(size_t)
    );

    NtQuerySystemInformation(
        SystemProcessorPerformanceInformation,
        p1,
        sizeof(*p1) * core_count,
        NULL
    );

    thread_usleep(100 * MILLI_MICRO);

    NtQuerySystemInformation(
        SystemProcessorPerformanceInformation,
        p2,
        sizeof(*p2) * core_count,
        NULL
    );

    for (int i = 0; i < core_count; ++i) {
        const LONGLONG idle = p2[i].IdleTime.QuadPart
            - p1[i].IdleTime.QuadPart;

        const LONGLONG total = (p2[i].KernelTime.QuadPart - p1[i].KernelTime.QuadPart)
            + (p2[i].UserTime.QuadPart   - p1[i].UserTime.QuadPart);

        usage[i] = 0.0f;
        if (total > 0.0f) {
            usage[i] = 1.0f - ((f32)idle / (f32)total);
        }
    }
}

inline
uint32 system_stack_usage()
{
    PROFILE_DEBUG(PROFILE_SYS_CALL);

    static thread_local const char* stack_base = NULL;

    if (!stack_base) {
        //NT_TIB* tib = (NT_TIB *) NtCurrentTeb();

        // This might be faster
        const NT_TIB* const tib = (NT_TIB *) __readgsqword(0x30);

        stack_base  = (char *) tib->StackBase;
    }

    // Find the current stack position
    volatile const char local_var = '0';

    return (uint32) (stack_base - (char *) &local_var);
}

inline
uint64 system_app_memory_usage()
{
    MEMORY_BASIC_INFORMATION mbi;
    SIZE_T address = 0;
    size_t total_size = 0;

    // MEM_IMAGE = DLL memory
    // MEM_MAPPED = Mapped files
    // VirtualQuery skips the handle resolution that VirtualQueryEx does, we always query our own process
    while (VirtualQuery((LPCVOID) address, &mbi, sizeof(mbi)) == sizeof(mbi)) {
        if (mbi.State == MEM_COMMIT && (mbi.Type == MEM_IMAGE || mbi.Type == MEM_MAPPED)) {
            total_size += mbi.RegionSize;
        }

        address += mbi.RegionSize;
    }

    return total_size;
}

/**
 * One 2 character part of the current UI locale name ("en-US" -> "en" / "US")
 *
 * The locale is parsed once per thread because it can't change while the process runs
 *
 * @return Locale name character pair, 0 = unknown
 */
static inline
uint16 system_locale_code(int32 offset) NO_EXCEPT
{
    static thread_local wchar_t local_name[LOCALE_NAME_MAX_LENGTH];
    static thread_local bool resolved = false;

    if (!resolved) {
        const LANGID lang_id = GetUserDefaultUILanguage();
        if (!LCIDToLocaleName(lang_id, local_name, LOCALE_NAME_MAX_LENGTH, 0)) {
            return 0;
        }

        resolved = true;
    }

    return (uint16) ((local_name[offset] << 8) | local_name[offset + 1]);
}

FORCE_INLINE
uint16 system_language_code()
{
    return system_locale_code(0);
}

FORCE_INLINE
uint16 system_country_code()
{
    return system_locale_code(3);
}

/**
 * Connection to the WMI namespace ROOT\CIMV2
 *
 * WARNING: Requires ole32_init() + wmi_guid_init()
 */
struct WmiConnection {
    IWbemLocator* locator;
    IWbemServices* services;
};

/**
 * Opens a WMI connection to ROOT\CIMV2, the caller has to call wmi_disconnect() when done
 *
 * @return false if COM or WMI isn't available, connection is unusable then
 */
static inline
bool wmi_connect(WmiConnection* const connection) NO_EXCEPT
{
    connection->locator = NULL;
    connection->services = NULL;

    if (FAILED(OLE32_CoCreateInstance(
        WBEMUUID_CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        WBEMUUID_IID_IWbemLocator,
        (LPVOID *) &connection->locator
    ))) {
        return false;
    }

    BSTR ns = SysAllocString(L"ROOT\\CIMV2");
    const HRESULT hres = connection->locator->ConnectServer(
        ns,
        NULL,
        NULL,
        0,
        NULL,
        0,
        0,
        &connection->services
    );

    SysFreeString(ns);

    if (FAILED(hres) || !connection->services) {
        connection->locator->Release();
        connection->locator = NULL;

        return false;
    }

    // Set security levels on the proxy
    if (FAILED(OLE32_CoSetProxyBlanket(
        connection->services,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE
    ))) {
        connection->services->Release();
        connection->services = NULL;

        connection->locator->Release();
        connection->locator = NULL;

        return false;
    }

    return true;
}

static inline
void wmi_disconnect(WmiConnection* const connection) NO_EXCEPT
{
    if (connection->services) {
        connection->services->Release();
        connection->services = NULL;
    }

    if (connection->locator) {
        connection->locator->Release();
        connection->locator = NULL;
    }
}

void mainboard_info_get(MainboardInfo* const info) {
    info->name[sizeof(info->name) - 1] = '\0';
    info->serial_number[sizeof(info->serial_number) - 1] = '\0';

    WmiConnection connection;
    if (!wmi_connect(&connection)) {
        return;
    }

    // Only select what is used, SELECT * marshals every Win32_BaseBoard property
    BSTR queryLanguage = SysAllocString(L"WQL");
    BSTR query = SysAllocString(L"SELECT Product, SerialNumber FROM Win32_BaseBoard");

    IEnumWbemClassObject* enumerator = NULL;
    const HRESULT hres = connection.services->ExecQuery(
        queryLanguage,
        query,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &enumerator
    );

    if (FAILED(hres)) {
        wmi_disconnect(&connection);

        SysFreeString(queryLanguage);
        SysFreeString(query);

        return;
    }

    // Retrieve the data
    while (enumerator) {
        IWbemClassObject* pclsObj = NULL;
        ULONG ret = 0;
        enumerator->Next(WBEM_INFINITE, 1, &pclsObj, &ret);
        if (ret == 0) {
            break;
        }

        VARIANT vtProp;
        HRESULT hr = pclsObj->Get(L"Product", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr)) {
            wchar_to_char(info->name, (wchar_t *) vtProp.bstrVal, sizeof(info->name) - 1);
            VariantClear(&vtProp);
        }

        hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr)) {
            wchar_to_char(info->serial_number, (wchar_t *) vtProp.bstrVal, sizeof(info->serial_number) - 1);
            VariantClear(&vtProp);
        }

        pclsObj->Release();
    }

    if (enumerator) {
        enumerator->Release();
    }

    wmi_disconnect(&connection);

    info->name[sizeof(info->name) - 1] = '\0';
    info->serial_number[sizeof(info->serial_number) - 1] = '\0';

    SysFreeString(queryLanguage);
    SysFreeString(query);
}

int32 network_info_get(NetworkInfo* const info, int32 limit = 4, RingMemory* ring = NULL) {
    const ULONG flags = GAA_FLAG_SKIP_UNICAST | GAA_FLAG_SKIP_ANYCAST
        | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;

    DWORD dwSize = 15 * 1024;

    // memory_get_temp doesn't move the ring head, so retrying on overflow doesn't waste ring space
    PIP_ADAPTER_ADDRESSES adapter_address = ring
        ? (PIP_ADAPTER_ADDRESSES) memory_get_temp(ring, dwSize, sizeof(size_t))
        : (PIP_ADAPTER_ADDRESSES) malloc(dwSize);

    if (!adapter_address) {
        return 0;
    }

    DWORD result = IPHLPAPI_GetAdaptersAddresses(AF_UNSPEC, flags, NULL, adapter_address, &dwSize);

    // 15 kb is only a guess, the API reports the required size on overflow
    while (result == ERROR_BUFFER_OVERFLOW) {
        if (ring) {
            adapter_address = (PIP_ADAPTER_ADDRESSES) memory_get_temp(ring, dwSize, sizeof(size_t));
        } else {
            PIP_ADAPTER_ADDRESSES grown = (PIP_ADAPTER_ADDRESSES) realloc(adapter_address, dwSize);
            if (!grown) {
                free(adapter_address);

                return 0;
            }

            adapter_address = grown;
        }

        if (!adapter_address) {
            return 0;
        }

        result = IPHLPAPI_GetAdaptersAddresses(AF_UNSPEC, flags, NULL, adapter_address, &dwSize);
    }

    if (result != NO_ERROR) {
        if (!ring) {
            free(adapter_address);
        }

        return 0;
    }

    int32 i = 0;
    for (PIP_ADAPTER_ADDRESSES adapter = adapter_address; adapter && i < limit; adapter = adapter->Next) {
        if (adapter->PhysicalAddressLength == 0) {
            continue;
        }

        // The mac is shorter than the buffer and the name conversion doesn't clear the remainder
        memset(info[i].slot, 0, sizeof(info[i].slot));
        memset(info[i].mac, 0, sizeof(info[i].mac));

        const size_t mac_len = adapter->PhysicalAddressLength < 8 ? adapter->PhysicalAddressLength : 8;
        memcpy(info[i].mac, adapter->PhysicalAddress, mac_len);

        // wcstombs is locale dependent and can leave the string unterminated
        WideCharToMultiByte(
            CP_UTF8,
            0,
            adapter->FriendlyName,
            -1,
            info[i].slot,
            (int32) sizeof(info[i].slot) - 1,
            NULL,
            NULL
        );

        ++i;
    }

    if (!ring) {
        free(adapter_address);
    }

    return i;
}

void cpu_info_get(CpuInfo* const info) {
    info->features = cpu_info_features();

    cpu_info_cache(1, &info->cache[0]);
    cpu_info_cache(2, &info->cache[1]);
    cpu_info_cache(3, &info->cache[2]);
    cpu_info_cache(4, &info->cache[3]);

    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    info->core_count = (uint16) sys_info.dwNumberOfProcessors;
    info->page_size = (uint16) sys_info.dwPageSize;

    int32 cpuInfo[4] = {0};
    __cpuid(cpuInfo, 0);

    memset(info->vendor, 0, sizeof(info->vendor));
    *((int32 *) info->vendor) = cpuInfo[1];
    *((int32 *) (info->vendor + 4)) = cpuInfo[3];
    *((int32 *) (info->vendor + 8)) = cpuInfo[2];
    info->vendor[12] = '\0';

    __cpuid(cpuInfo, 0x80000002);
    memcpy(info->brand, cpuInfo, sizeof(cpuInfo));
    __cpuid(cpuInfo, 0x80000003);
    memcpy(info->brand + 16, cpuInfo, sizeof(cpuInfo));
    __cpuid(cpuInfo, 0x80000004);
    memcpy(info->brand + 32, cpuInfo, sizeof(cpuInfo));
    info->brand[48] = '\0';

    __cpuid(cpuInfo, 1);
    info->model = (cpuInfo[0] >> 4) & 0xF;
    info->family = (cpuInfo[0] >> 8) & 0xF;

    DWORD bufSize = sizeof(DWORD);
    HKEY hKey = NULL;
    info->mhz = 0;

    const long lError = ADVAPI32_RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0, KEY_READ, &hKey
    );

    if (lError == ERROR_SUCCESS) {
        ADVAPI32_RegQueryValueExW(hKey, L"~MHz", NULL, NULL, (LPBYTE) &(info->mhz), &bufSize);

        ADVAPI32_RegCloseKey(hKey);
    }
}

// RtlGetVersion is exported by ntdll and bound through the ntdll.lib import lib,
// but the SDK only declares it in the kernel-mode headers (ntddk.h)
extern "C" NTSTATUS NTAPI RtlGetVersion(OSVERSIONINFOEXW* version_info);

inline
void os_info_get(OSInfo* const info) {
    info->vendor[15] = '\0';
    info->name[63] = '\0';

    OSVERSIONINFOEXW version_info;
    memset(&version_info, 0, sizeof(OSVERSIONINFOEXW));

    version_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

    RtlGetVersion(&version_info);

    memcpy(info->vendor, "Microsoft", sizeof("Microsoft"));
    memcpy(info->name, "Windows", sizeof("Windows"));
    info->major = version_info.dwMajorVersion;
    info->minor = version_info.dwMinorVersion;
}

FORCE_INLINE
void ram_info_get(RamInfo* const info) {
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    info->memory = (uint32) (statex.ullTotalPhys / (1024 * 1024));
}

/**
 * Memory channel of a ram module from its WMI description, e.g.
 *      BankLabel:      "P0 CHANNEL A", "CHANNEL B"
 *      DeviceLocator:  "ChannelA-DIMM0", "DIMM_A1", "DIMMB2"
 *
 * @return Channel letter ('A', 'B', ...), 0 = unknown
 */
static inline
wchar_t ram_module_channel(const wchar_t* str) NO_EXCEPT
{
    if (!str) {
        return 0;
    }

    for (; *str; ++str) {
        // "CHANNEL X", "Channel_X", "ChannelX"
        const wchar_t* channel = L"CHANNEL";
        const wchar_t* pos = str;
        while (*channel && (*pos == *channel || *pos == *channel + (L'a' - L'A'))) {
            ++pos;
            ++channel;
        }

        if (!*channel) {
            while (*pos == L' ' || *pos == L'_' || *pos == L'-') {
                ++pos;
            }

            if (*pos >= L'a' && *pos <= L'z') {
                return (wchar_t) (*pos - (L'a' - L'A'));
            } else if ((*pos >= L'A' && *pos <= L'Z') || (*pos >= L'0' && *pos <= L'9')) {
                return *pos;
            }

            return 0;
        }

        // "DIMM_A1", "DIMMA1" (letter = channel, number = slot)
        const wchar_t* dimm = L"DIMM";
        pos = str;
        while (*dimm && (*pos == *dimm || *pos == *dimm + (L'a' - L'A'))) {
            ++pos;
            ++dimm;
        }

        if (!*dimm) {
            if (*pos == L'_') {
                ++pos;
            }

            if (*pos >= L'A' && *pos <= L'H' && pos[1] >= L'0' && pos[1] <= L'9') {
                return *pos;
            }
        }
    }

    return 0;
}

/**
 * Checks if the ram modules use more than one memory channel
 *
 * The result is only an estimate based on the module descriptions of the bios (WMI Win32_PhysicalMemory)
 * If the channels can't be identified we assume that multiple modules are in dual channel (no false warnings)
 *
 * Can be called from any thread (initializes COM for the calling thread if necessary)
 * WARNING: Requires ole32_init() + wmi_guid_init()
 */
RamChannelType ram_channel_info() NO_EXCEPT
{
    // S_FALSE = already initialized on this thread (still needs a matching CoUninitialize)
    // RPC_E_CHANGED_MODE = already initialized with a different mode (usable, but no CoUninitialize)
    const bool com_initialized = SUCCEEDED(OLE32_CoInitializeEx(NULL, COINIT_MULTITHREADED));

    RamChannelType result = RAM_CHANNEL_TYPE_FAILED;

    WmiConnection connection;
    if (wmi_connect(&connection)) {
        BSTR query_language = SysAllocString(L"WQL");
        BSTR query = SysAllocString(L"SELECT BankLabel, DeviceLocator FROM Win32_PhysicalMemory");

        IEnumWbemClassObject* enumerator = NULL;
        if (SUCCEEDED(connection.services->ExecQuery(
            query_language,
            query,
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &enumerator
        ))) {
            int32 module_count = 0;

            // Channel of the first module, all_same_channel = every module has a known channel and it is the same one
            wchar_t first_channel = 0;
            bool all_same_channel = true;

            while (true) {
                IWbemClassObject* object = NULL;
                ULONG returned = 0;
                if (FAILED(enumerator->Next(WBEM_INFINITE, 1, &object, &returned)) || returned == 0) {
                    break;
                }

                ++module_count;

                wchar_t channel = 0;
                VARIANT value;

                if (SUCCEEDED(object->Get(L"BankLabel", 0, &value, 0, 0))) {
                    if (value.vt == VT_BSTR) {
                        channel = ram_module_channel(value.bstrVal);
                    }

                    VariantClear(&value);
                }

                if (!channel && SUCCEEDED(object->Get(L"DeviceLocator", 0, &value, 0, 0))) {
                    if (value.vt == VT_BSTR) {
                        channel = ram_module_channel(value.bstrVal);
                    }

                    VariantClear(&value);
                }

                if (module_count == 1) {
                    first_channel = channel;
                }

                all_same_channel = all_same_channel && channel && channel == first_channel;

                object->Release();
            }

            if (module_count == 1) {
                result = RAM_CHANNEL_TYPE_SINGLE_CHANNEL;
            } else if (module_count > 1) {
                // Multiple modules in the same channel -> moving one of them to another slot enables dual channel
                result = all_same_channel ? RAM_CHANNEL_TYPE_CAN_UPGRADE : RAM_CHANNEL_TYPE_DUAL_CHANNEL;
            }
        }

        if (enumerator) {
            enumerator->Release();
        }

        SysFreeString(query);
        SysFreeString(query_language);

        wmi_disconnect(&connection);
    }

    if (com_initialized) {
        OLE32_CoUninitialize();
    }

    return result;
}

inline
int32 gpu_info_get(GpuInfo*const  info, int32 limit = 3) {
    // The 1.1 factory is needed for DXGI_ADAPTER_DESC1.Flags (only place that marks WARP adapters)
    IDXGIFactory1* factory = NULL;
    IDXGIAdapter1* adapter = NULL;
    DXGI_ADAPTER_DESC1 adapter_description;

    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void **) &factory);
    if (FAILED(hr)) {
        return 0;
    }

    int32 i = 0;

    // limit counts the gpus we keep, not the adapters we visit (software adapters are skipped)
    for (int32 adapter_index = 0; i < limit; ++adapter_index) {
        if (factory->EnumAdapters1(adapter_index, &adapter) == DXGI_ERROR_NOT_FOUND) {
            break;
        }

        hr = adapter->GetDesc1(&adapter_description);
        if (FAILED(hr)) {
            adapter->Release();
            break;
        }

        // WARP / Basic Render Driver is not a gpu the player can use
        if (adapter_description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            adapter->Release();
            continue;
        }

        wchar_to_char(
            info[i].name,
            adapter_description.Description,
            (int32) ARRAY_COUNT(info[i].name) - 1
        );
        info[i].vram = (uint32) (adapter_description.DedicatedVideoMemory / (1024 * 1024));

        adapter->Release();
        ++i;
    }

    factory->Release();

    return i;
}

inline
int32 display_info_get(DisplayInfo* const info, int32 limit = 4) {
    DISPLAY_DEVICEW device;
    DEVMODEW mode = {0};

    device.cb = sizeof(DISPLAY_DEVICEW);

    int32 i = 0;
    int32 j = 0;

    // We limit the display detection to a maximum of 100
    while (EnumDisplayDevicesW(NULL, j, &device, 0) && i < limit && ++j < 100) {
        // Mirroring/disconnected pseudo adapters have no usable mode and EnumDisplaySettingsW is the expensive part
        if (!(device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
            || (device.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
        ) {
            continue;
        }

        mode.dmSize = sizeof(mode);

        if (EnumDisplaySettingsW(device.DeviceName, ENUM_CURRENT_SETTINGS, &mode)) {
            wchar_to_char(info[i].name, device.DeviceName, (int32) ARRAY_COUNT(info[i].name) - 1);

            info[i].width = mode.dmPelsWidth;
            info[i].height = mode.dmPelsHeight;
            info[i].hz = mode.dmDisplayFrequency;
            info[i].is_primary = (bool) (device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE);
            ++i;
        }
    }

    return i;
}

/**
 * Checks if a monitor is connected to a dedicated gpu
 *
 * Returns false if the monitor is connected to the mainboard (integrated gpu) even if a dedicated gpu exists
 * (also the case for most laptops with hybrid graphics)
 */
inline
bool is_dedicated_gpu_connected() NO_EXCEPT
{
    DISPLAY_DEVICEW adapter;
    adapter.cb = sizeof(DISPLAY_DEVICEW);

    // The first level of EnumDisplayDevicesW returns the adapters (gpu outputs), their DeviceID is the pci id
    // (the second level would return the monitors with "MONITOR\..." ids)
    for (DWORD i = 0; EnumDisplayDevicesW(NULL, i, &adapter, 0); ++i) {
        if (!(adapter.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) {
            continue;
        }

        if (str_contains(adapter.DeviceID, L"PCI\\VEN_10DE") // Nvidia
            || str_contains(adapter.DeviceID, L"PCI\\VEN_1002") // AMD
            || (str_contains(adapter.DeviceID, L"PCI\\VEN_8086") && str_contains(adapter.DeviceID, L"DEV_56")) // Intel Arc
            || str_contains(adapter.DeviceID, L"PCI\\VEN_1E4E") // Moore Threads
            || str_contains(adapter.DeviceID, L"PCI\\VEN_1DBA") // Innosilicon
            || str_contains(adapter.DeviceID, L"PCI\\VEN_1D17") // Zhaoxin
            // @todo what about Biren and Intel Arc B-series (DEV_E2..)?
        ) {
            return true;
        }
    }

    return false;
}

/**
 * Checks if the primary monitor supports a higher refresh rate than the current one at the current resolution
 */
bool monitor_supports_higher_hz() NO_EXCEPT
{
    DEVMODEW current = {};
    current.dmSize = sizeof(DEVMODEW);

    if (!EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &current) || current.dmDisplayFrequency <= 1) {
        // 0 and 1 = hardware default refresh rate
        return false;
    }

    DEVMODEW mode = {};
    mode.dmSize = sizeof(DEVMODEW);

    // Higher refresh rates at a lower resolution don't count (e.g. 4k@60 vs 1080p@120)
    for (DWORD i = 0; EnumDisplaySettingsW(NULL, i, &mode); ++i) {
        if (mode.dmPelsWidth == current.dmPelsWidth
            && mode.dmPelsHeight == current.dmPelsHeight
            && mode.dmDisplayFrequency > current.dmDisplayFrequency
        ) {
            return true;
        }
    }

    return false;
}

#endif