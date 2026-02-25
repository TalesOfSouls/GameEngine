/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_SYSTEM_INFO_C
#define COMS_PLATFORM_WIN32_SYSTEM_INFO_C

#include "../../stdlib/Stdlib.h"
#include "../../utils/StringUtils.h"
#include "../../system/SystemInfo.h"
#include "../../architecture/CpuInfo.cpp"

#include <psapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wbemidl.h>
#include <comdef.h>
#include <winnls.h>
#include <wingdi.h>
#include <hidsdi.h>

#include "libs/ole32_static.h"
#include "libs/cfgmgr32.h"
#include "libs/setupapi.h"
#include "libs/iphlpapi.h"
#include "libs/comsuppw.h"
#include "libs/wbemuuid.h"
#include "libs/Advapi32.h"

#include <intrin.h>
//#pragma comment(lib, "Advapi32.lib")
//#pragma comment(lib, "wbemuuid.lib")
//#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Ws2_32.lib")
//#pragma comment(lib, "setupapi.lib")
//#pragma comment(lib, "cfgmgr32.lib")
//#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "ntdll.lib")

FORCE_INLINE
uint64 system_private_memory_usage()
{
    PROCESS_MEMORY_COUNTERS_EX pmc;
    const HANDLE process = GetCurrentProcess();

    GetProcessMemoryInfo(process, (PROCESS_MEMORY_COUNTERS *) &pmc, sizeof(pmc));

    CloseHandle(process);

    return pmc.PrivateUsage;
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

    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION* p1 = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *) ring_get_memory(
        ring,
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * core_count,
        sizeof(size_t)
    );
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION* p2 = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *) ring_get_memory(
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

    Sleep(100);

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
    HRESULT hres = CoCreateInstance(
        pCLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        pIID_IWbemLocator,
        (LPVOID *)&pLoc
    );

    if (FAILED(hres)) {
        return;
    }

    // Connect to WMI through IWbemLocator::ConnectServer
    IWbemServices* pSvc = NULL;
    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
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
        return;
    }

    // Set security levels on the proxy
    hres = CoSetProxyBlanket(
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
        return;
    }

    // Use the IWbemServices pointer to make a WMI query
    IEnumWbemClassObject* enumerator = NULL;
    hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_BaseBoard"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &enumerator
    );

    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
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
}

int32 network_info_get(NetworkInfo* const info, int32 limit = 4, RingMemory* ring = NULL) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return 0;
    }

    DWORD dwSize = 0;
    PIP_ADAPTER_ADDRESSES adapter_address = NULL;
    PIP_ADAPTER_ADDRESSES adapter = NULL;

    // Get the size of the adapter addresses buffer
    if (pGetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &dwSize) == ERROR_BUFFER_OVERFLOW) {
        WSACleanup();
        return 0;
    }

    if (ring) {
        adapter_address = (PIP_ADAPTER_ADDRESSES) ring_get_memory(ring, sizeof(*adapter_address), sizeof(size_t));
    } else {
        adapter_address = (PIP_ADAPTER_ADDRESSES) malloc(dwSize);
    }

    if (!adapter_address) {
        WSACleanup();
        return 0;
    }

    // Get the adapter addresses
    if (pGetAdaptersAddresses(AF_UNSPEC, 0, NULL, adapter_address, &dwSize) != NO_ERROR) {
        if (!ring) {
            free(adapter_address);
        }

        WSACleanup();
        return 0;
    }

    int32 i = 0;

    // Iterate over the adapters and print their MAC addresses
    adapter = adapter_address;
    while (adapter && i < limit) {
        if (adapter->PhysicalAddressLength != 0) {
            info[i].slot[63] = '\0';
            info[i].mac[23] = '\0';

            memcpy(info[i].mac, adapter->PhysicalAddress, 8);
            wcstombs(info[i].slot, adapter->FriendlyName, 63);

            ++i;
        }

        adapter = adapter->Next;
    }

    if (!ring) {
        free(adapter_address);
    }

    WSACleanup();

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
    const long lError = pRegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0, KEY_READ, &hKey
    );

    if (lError == ERROR_SUCCESS) {
        pRegQueryValueExW(hKey, L"~MHz", NULL, NULL, (LPBYTE) &(info->mhz), &bufSize);
    }

    pRegCloseKey(hKey);
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

RamChannelType ram_channel_info() {
    IWbemLocator* pLoc = NULL;
    HRESULT hres = CoCreateInstance(pCLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, pIID_IWbemLocator, (LPVOID *)&pLoc);
    if (FAILED(hres)) {

        return RAM_CHANNEL_TYPE_FAILED;
    }

    IWbemServices* pSvc = NULL;
    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hres)) {
        pLoc->Release();

        return RAM_CHANNEL_TYPE_FAILED;
    }

    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();

        return RAM_CHANNEL_TYPE_FAILED;
    }

    IEnumWbemClassObject* enumerator = NULL;
    hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_PhysicalMemory"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &enumerator);
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();

        return RAM_CHANNEL_TYPE_FAILED;
    }

    IWbemClassObject* pclsObj = NULL;
    ULONG ret = 0;
    int32 ram_module_count = 0;
    int32 dual_channel_capable = 0;

    while (enumerator) {
        enumerator->Next(WBEM_INFINITE, 1, &pclsObj, &ret);
        if (ret == 0) break;

        VARIANT vtProp;
        hres = pclsObj->Get(L"BankLabel", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hres)) {
            ++ram_module_count;
            if (wcscmp(vtProp.bstrVal, L"BANK 0") == 0 || wcscmp(vtProp.bstrVal, L"BANK 1") == 0) {
                dual_channel_capable = 1;
            }

            VariantClear(&vtProp);
        }
        pclsObj->Release();
    }

    pSvc->Release();
    pLoc->Release();
    CoUninitialize();

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
    DEVMODEW mode;

    device.cb = sizeof(DISPLAY_DEVICEW);

    int32 i = 0;
    while (EnumDisplayDevicesW(NULL, i, &device, 0) && i < limit) {
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

inline
bool is_dedicated_gpu_connected() {
    DISPLAY_DEVICEW display_device;
    display_device.cb = sizeof(DISPLAY_DEVICEW);
    for (int32 i = 0; EnumDisplayDevicesW(NULL, i, &display_device, 0); ++i) {
        if (display_device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) {
            DISPLAY_DEVICEW gpuDevice;
            gpuDevice.cb = sizeof(DISPLAY_DEVICEW);
            if (EnumDisplayDevicesW(display_device.DeviceName, 0, &gpuDevice, 0)) {
                if (gpuDevice.DeviceID
                    && (str_contains(gpuDevice.DeviceID, L"PCI\\VEN_10DE") // Nvidia
                        || str_contains(gpuDevice.DeviceID, L"PCI\\VEN_1002") // AMD
                        || (str_contains(gpuDevice.DeviceID, L"PCI\\VEN_8086") && str_contains(gpuDevice.DeviceID, L"DEV_56")) // Intel
                        || str_contains(gpuDevice.DeviceID, L"PCI\\VEN_1E4E") // Moore Threads
                        || str_contains(gpuDevice.DeviceID, L"PCI\\VEN_1DBA") // Innosilicon
                        || str_contains(gpuDevice.DeviceID, L"PCI\\VEN_1D17") // Zhaoxin
                        // @todo what about Biren?
                    )
                ) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool monitor_supports_higher_hz()
{
    DEVMODE current = {0};
    current.dmSize = sizeof(DEVMODE);

    if (!EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &current)) {
        return false;
    }

    const DWORD current_hz = current.dmDisplayFrequency;
    if (current_hz <= 0) {
        return false;
    }

    DEVMODE mode = {0};
    mode.dmSize = sizeof(DEVMODE);

    for (int i = 0; EnumDisplaySettingsW(NULL, i, &mode); ++i) {
        if (mode.dmDisplayFrequency > current_hz) {
            return true;
        }
    }

    return false;
}

#endif