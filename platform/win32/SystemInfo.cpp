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

    return freq.QuadPart > 0 && !is_empty(buffer, sizeof(result))
        ? (f64) (end.QuadPart - start.QuadPart) / freq.QuadPart
        : 0.0;
}

FORCE_INLINE
uint64 system_private_memory_usage() NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_SYS_CALL);
    PROCESS_MEMORY_COUNTERS_EX pmc;
    const HANDLE process = GetCurrentProcess();

    GetProcessMemoryInfo(process, (PROCESS_MEMORY_COUNTERS *) &pmc, sizeof(pmc));

    CloseHandle(process);

    return pmc.PrivateUsage;
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
typedef NTSTATUS (NTAPI* NtQuerySystemInformation_t)(
    SYSTEM_INFORMATION_CLASS,
    PVOID,
    ULONG,
    PULONG
);

void system_cpu_usage(f32* const __restrict usage, int32 core_count, RingMemory* const __restrict ring) {
    NtQuerySystemInformation_t NtQuerySystemInformation = (NtQuerySystemInformation_t) GetProcAddress(
        GetModuleHandleW(L"ntdll.dll"),
        "NtQuerySystemInformation"
    );

    if (!NtQuerySystemInformation) {
        return;
    };

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
    while (VirtualQueryEx(GetCurrentProcess(), (LPCVOID) address, &mbi, sizeof(mbi)) == sizeof(mbi)) {
        if (mbi.State == MEM_COMMIT && (mbi.Type == MEM_IMAGE || mbi.Type == MEM_MAPPED)) {
            total_size += mbi.RegionSize;
        }

        address += mbi.RegionSize;
    }

    return total_size;
}

FORCE_INLINE
uint16 system_language_code()
{
    const LANGID lang_id = GetUserDefaultUILanguage();
    wchar_t local_name[LOCALE_NAME_MAX_LENGTH];

    if (!LCIDToLocaleName(lang_id, local_name, LOCALE_NAME_MAX_LENGTH, 0)) {
        return 0;
    }

    return (local_name[0] << 8) | local_name[1];
}

FORCE_INLINE
uint16 system_country_code()
{
    const LANGID lang_id = GetUserDefaultUILanguage();
    wchar_t local_name[LOCALE_NAME_MAX_LENGTH];

    if (!LCIDToLocaleName(lang_id, local_name, LOCALE_NAME_MAX_LENGTH, 0)) {
        return 0;
    }

    return (local_name[3] << 8) | local_name[4];
}

void mainboard_info_get(MainboardInfo* const info) {
    info->name[sizeof(info->name) - 1] = '\0';
    info->serial_number[sizeof(info->serial_number) - 1] = '\0';

    // Obtain initial locator to WMI
    IWbemLocator* pLoc = NULL;
    HRESULT hres = OLE32_CoCreateInstance(
        WBEMUUID_CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        WBEMUUID_IID_IWbemLocator,
        (LPVOID *)&pLoc
    );

    if (FAILED(hres)) {
        return;
    }

    // Connect to WMI through IWbemLocator::ConnectServer
    BSTR ns = SysAllocString(L"ROOT\\CIMV2");
    IWbemServices* pSvc = NULL;
    hres = pLoc->ConnectServer(
        ns,
        NULL,
        NULL,
        0,
        NULL,
        0,
        0,
        &pSvc
    );

    if (FAILED(hres)) {
        pLoc->Release();
        SysFreeString(ns);
        return;
    }

    // Set security levels on the proxy
    hres = OLE32_CoSetProxyBlanket(
        pSvc,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE
    );

    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        SysFreeString(ns);
        return;
    }

    // Use the IWbemServices pointer to make a WMI query
    BSTR queryLanguage = SysAllocString(L"WQL");
    BSTR query = SysAllocString(L"SELECT * FROM Win32_BaseBoard");

    IEnumWbemClassObject* enumerator = NULL;
    hres = pSvc->ExecQuery(
        queryLanguage,
        query,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &enumerator
    );

    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();

        SysFreeString(queryLanguage);
        SysFreeString(query);
        SysFreeString(ns);
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

    // Clean up
    pSvc->Release();
    pLoc->Release();
    enumerator->Release();

    info->name[sizeof(info->name) - 1] = '\0';
    info->serial_number[sizeof(info->serial_number) - 1] = '\0';

    SysFreeString(queryLanguage);
    SysFreeString(query);
    SysFreeString(ns);
}

int32 network_info_get(NetworkInfo* const info, int32 limit = 4, RingMemory* ring = NULL) {
    const ULONG flags = GAA_FLAG_SKIP_UNICAST | GAA_FLAG_SKIP_ANYCAST
        | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;

    DWORD dwSize = 15 * 1024;
    PIP_ADAPTER_ADDRESSES adapter_address = ring
        ? (PIP_ADAPTER_ADDRESSES) memory_get(ring, dwSize, sizeof(size_t))
        : (PIP_ADAPTER_ADDRESSES) malloc(dwSize);

    if (!adapter_address) {
        return 0;
    }

    DWORD result = IPHLPAPI_GetAdaptersAddresses(AF_UNSPEC, flags, NULL, adapter_address, &dwSize);
    if (result == ERROR_BUFFER_OVERFLOW) {
        if (!ring) {
            free(adapter_address);
        }

        adapter_address = ring
            ? (PIP_ADAPTER_ADDRESSES) memory_get(ring, dwSize, sizeof(size_t))
            : (PIP_ADAPTER_ADDRESSES) malloc(dwSize);

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
        if (adapter->PhysicalAddressLength != 0) {
            info[i].slot[63] = '\0';
            info[i].mac[23] = '\0';

            size_t mac_len = adapter->PhysicalAddressLength < 8 ? adapter->PhysicalAddressLength : 8;
            memcpy(info[i].mac, adapter->PhysicalAddress, mac_len);
            wcstombs(info[i].slot, adapter->FriendlyName, 63);
            ++i;
        }
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
    HKEY hKey;
    const long lError = ADVAPI32_RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0, KEY_READ, &hKey
    );

    if (lError == ERROR_SUCCESS) {
        ADVAPI32_RegQueryValueExW(hKey, L"~MHz", NULL, NULL, (LPBYTE) &(info->mhz), &bufSize);
    }

    ADVAPI32_RegCloseKey(hKey);
}

inline
void os_info_get(OSInfo* const info) {
    info->vendor[15] = '\0';
    info->name[63] = '\0';

    OSVERSIONINFOEXW version_info;
    memset(&version_info, 0, sizeof(OSVERSIONINFOEXW));

    version_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    NTSTATUS(WINAPI *RtlGetVersion)(OSVERSIONINFOEXW*) = (NTSTATUS(WINAPI *)(OSVERSIONINFOEXW*))GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");

    if (RtlGetVersion != nullptr) {
        RtlGetVersion(&version_info);
    }

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

    IWbemLocator* locator = NULL;
    IWbemServices* services = NULL;
    IEnumWbemClassObject* enumerator = NULL;

    BSTR ns = SysAllocString(L"ROOT\\CIMV2");
    BSTR query_language = SysAllocString(L"WQL");
    BSTR query = SysAllocString(L"SELECT BankLabel, DeviceLocator FROM Win32_PhysicalMemory");

    if (SUCCEEDED(OLE32_CoCreateInstance(WBEMUUID_CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, WBEMUUID_IID_IWbemLocator, (LPVOID *) &locator))
        && SUCCEEDED(locator->ConnectServer(ns, NULL, NULL, 0, 0, 0, 0, &services))
        && SUCCEEDED(OLE32_CoSetProxyBlanket(services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE))
        && SUCCEEDED(services->ExecQuery(query_language, query, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &enumerator))
    ) {
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

    if (services) {
        services->Release();
    }

    if (locator) {
        locator->Release();
    }

    SysFreeString(query);
    SysFreeString(query_language);
    SysFreeString(ns);

    if (com_initialized) {
        OLE32_CoUninitialize();
    }

    return result;
}

inline
int32 gpu_info_get(GpuInfo*const  info, int32 limit = 3) {
    IDXGIFactory* factory = NULL;
    IDXGIAdapter* adapter = NULL;
    DXGI_ADAPTER_DESC adapter_description;

    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void **) &factory);
    if (FAILED(hr)) {
        return 0;
    }

    int32 i = 0;
    while (factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND && i < limit) {
        hr = adapter->GetDesc(&adapter_description);
        if (FAILED(hr)) {
            adapter->Release();
            break;
        }

        wchar_to_char(info[i].name, adapter_description.Description, 63);
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
        mode.dmSize = sizeof(mode);

        if (EnumDisplaySettingsW(device.DeviceName, ENUM_CURRENT_SETTINGS, &mode)) {
            wchar_to_char(info[i].name, device.DeviceName);

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